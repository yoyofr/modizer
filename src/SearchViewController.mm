    //
//  SearchViewController.m
//  modizer4
//
//  Created by yoyofr on 7/16/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

enum {
    PLAYLIST_SEARCH=1,
    LOCAL_SEARCH,
    MODLAND_SEARCH,
    HVSC_SEARCH,
    ASMA_SEARCH
};
static int lastSelectedSearch;

#include <pthread.h>
extern pthread_mutex_t db_mutex;

#import "SearchViewController.h"
#import <QuartzCore/CAGradientLayer.h>
#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];

#define MAX_SEARCH_RESULT 512   //per section

static volatile int mSearchProgress;
static volatile int mSearchMode;
static int local_flag;

static NSFileManager *mFileMngr;

@implementation SearchViewController 

@synthesize tableView,sBar,detailViewController,searchPrgView,searchLabel;
@synthesize downloadViewController,rootViewControllerIphone;
@synthesize popTipView;

#include "MiniPlayerImplementTableView.h"

-(IBAction)goPlayer {
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
        UIAlertView *nofileplaying=[[UIAlertView alloc] initWithTitle:@"Warning"
                                                               message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
        [nofileplaying show];
    }
}

-(void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer {
    CGPoint p = [gestureRecognizer locationInView:self.tableView];
    
    NSIndexPath *indexPath = [self.tableView indexPathForRowAtPoint:p];
    if (indexPath != nil) {
        if ((gestureRecognizer.state==UIGestureRecognizerStateBegan)||(gestureRecognizer.state==UIGestureRecognizerStateChanged)) {
            int crow=indexPath.row;
            int csection=indexPath.section;
            NSString *str=nil;
            
            switch (csection) {
                case 0:
                    str=[NSString stringWithFormat:@"%@", playlist_entries[indexPath.row].playlist_name];
                    break;
                case 1:
                    str=local_entries[crow].fullpath;
                    break;
                case 2:
                    str=db_entries[crow].fullpath;
                    break;
                case 3:
                    str=dbHVSC_entries[crow].fullpath;
                    break;
                case 4:
                    str=dbASMA_entries[crow].fullpath;
                    break;
            }
            
            if (str) {
                //display popup
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

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
	clock_t start_time,end_time;	
	start_time=clock();
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    /////////////////////////////////////
    // Waiting view
    /////////////////////////////////////
    waitingView = [[WaitingView alloc] init];
    [self.view addSubview:waitingView];
    
    NSDictionary *views = NSDictionaryOfVariableBindings(waitingView);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingView(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingView(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    
    lastSelectedSearch=0;
    
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithCustomView: btn];
    self.navigationItem.rightBarButtonItem = item;

    mFileMngr=[[NSFileManager alloc] init];
	
	modland_searchOn=0;
	HVSC_searchOn=0;
    ASMA_searchOn=0;
	playlist_searchOn=0;
	local_searchOn=0;
	
	self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	//self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
    //tableView.rowHeight = 28;
	tableView.sectionHeaderHeight = 32;
    //self.tableView.backgroundColor = [UIColor clearColor];
    
    popTipViewRow=-1;
    popTipViewSection=-1;
    UILongPressGestureRecognizer *lpgr = [[UILongPressGestureRecognizer alloc]
                                          initWithTarget:self action:@selector(handleLongPress:)];
    lpgr.minimumPressDuration = 1.0; //seconds
    lpgr.delegate = self;
    [self.tableView addGestureRecognizer:lpgr];
    //[lpgr release];
    
	
	
	mSearch=0;
	mSearchText=nil;
	
	dbASMA_entries_count=dbHVSC_entries_count=db_entries_count=local_entries_count=playlist_entries_count=0;
	db_entries=NULL;
    dbASMA_entries=NULL;
	dbHVSC_entries=NULL;
	local_entries=NULL;
	playlist_entries=NULL;
	tooMuchDB=tooMuchPL=tooMuchLO=tooMuchDBASMA=tooMuchDBHVSC=0;
	
	ASMA_expanded=HVSC_expanded=modland_expanded=local_expanded=playlist_expanded=0;
    [super viewDidLoad];
	
	end_time=clock();	
#ifdef LOAD_PROFILE
	NSLog(@"search : %d",end_time-start_time);
#endif
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

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleDefault;
}

- (void)viewWillAppear:(BOOL)animated {
    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    [self.sBar setBarStyle:UIBarStyleDefault];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
        
    [self hideWaiting];
    
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    
    
    for (int i=0;i<dbASMA_entries_count;i++) dbASMA_entries[i].downloaded=-1;
    for (int i=0;i<dbHVSC_entries_count;i++) dbHVSC_entries[i].downloaded=-1;
    for (int i=0;i<db_entries_count;i++) db_entries[i].downloaded=-1;
    
    if ([detailViewController mPlaylist_size]>0) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    } else {
        wasMiniPlayerOn=false;
        [self hideMiniPlayer];
    }
    
	[tableView reloadData];
	
	[super viewWillAppear:animated];
}
- (void)viewWillDisappear:(BOOL)animated {
	
	[super viewWillDisappear:animated];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
	[tableView reloadData];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [self.tableView reloadData];
    [miniplayerVC viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}


// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return YES;
}


- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void)dealloc {
    [waitingView removeFromSuperview];
    waitingView=nil;
    
	if (dbHVSC_entries_count) {
		for (int i=0;i<dbHVSC_entries_count;i++) {
			/*if (dbHVSC_entries[i].label) [dbHVSC_entries[i].label release];
			if (dbHVSC_entries[i].fullpath) [dbHVSC_entries[i].fullpath release];
			if (dbHVSC_entries[i].id_md5) [dbHVSC_entries[i].id_md5 release];
			if (dbHVSC_entries[i].dir1) [dbHVSC_entries[i].dir1 release];
			if (dbHVSC_entries[i].dir2) [dbHVSC_entries[i].dir2 release];
			if (dbHVSC_entries[i].dir3) [dbHVSC_entries[i].dir3 release];
			if (dbHVSC_entries[i].dir4) [dbHVSC_entries[i].dir4 release];
			if (dbHVSC_entries[i].dir5) [dbHVSC_entries[i].dir5 release];*/
            dbHVSC_entries[i].label=nil;
            dbHVSC_entries[i].fullpath=nil;
            dbHVSC_entries[i].id_md5=nil;
            dbHVSC_entries[i].dir1=nil;
            dbHVSC_entries[i].dir2=nil;
            dbHVSC_entries[i].dir3=nil;
            dbHVSC_entries[i].dir4=nil;
            dbHVSC_entries[i].dir5=nil;
		}
		free(dbHVSC_entries);
		dbHVSC_entries=NULL;
		dbHVSC_entries_count=0;
	}
    if (dbASMA_entries_count) {
		for (int i=0;i<dbASMA_entries_count;i++) {
			/*if (dbASMA_entries[i].label) [dbASMA_entries[i].label release];
			if (dbASMA_entries[i].fullpath) [dbASMA_entries[i].fullpath release];
			if (dbASMA_entries[i].id_md5) [dbASMA_entries[i].id_md5 release];
			if (dbASMA_entries[i].dir1) [dbASMA_entries[i].dir1 release];
			if (dbASMA_entries[i].dir2) [dbASMA_entries[i].dir2 release];
			if (dbASMA_entries[i].dir3) [dbASMA_entries[i].dir3 release];
			if (dbASMA_entries[i].dir4) [dbASMA_entries[i].dir4 release];
			*/
            dbASMA_entries[i].label=nil;
            dbASMA_entries[i].fullpath=nil;
            dbASMA_entries[i].id_md5=nil;
            dbASMA_entries[i].dir1=nil;
            dbASMA_entries[i].dir2=nil;
            dbASMA_entries[i].dir3=nil;
            dbASMA_entries[i].dir4=nil;
		}
		free(dbASMA_entries);
		dbASMA_entries=NULL;
		dbASMA_entries_count=0;
	}
	if (db_entries_count) {
		for (int j=0;j<db_entries_count;j++) {
			//[db_entries[j].label release];
			//[db_entries[j].fullpath release];
		}
		free(db_entries);
		db_entries=NULL;
		db_entries_count=0;
	}
	if (playlist_entries_count) {
		for (int j=0;j<playlist_entries_count;j++) {
            playlist_entries[j].playlist_name=nil;
            playlist_entries[j].playlist_id=nil;
            playlist_entries[j].filename=nil;
            playlist_entries[j].fullpath=nil;
		}
		playlist_entries_count=0;
	}
	if (local_entries_count) {
		for (int j=0;j<local_entries_count;j++) {
            local_entries[j].label=nil;
            local_entries[j].fullpath=nil;
		}
		free(local_entries);
		local_entries=NULL;
		local_entries_count=0;
	}
    //[super dealloc];
}

-(int) searchPlaylist {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int playlist_entries_idx;
	
	if (playlist_entries_count) {
		for (int j=0;j<playlist_entries_count;j++) {
            playlist_entries[j].playlist_name=nil;
            playlist_entries[j].playlist_id=nil;
            playlist_entries[j].filename=nil;
            playlist_entries[j].fullpath=nil;

		}
		playlist_entries_count=0;
	}
	pthread_mutex_lock(&db_mutex);
	playlist_entries_idx=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		sprintf(sqlStatement,"SELECT p.name,p.id,pe.name,pe.fullpath FROM playlists_entries pe,playlists p \
				WHERE pe.id_playlist=p.id \
				AND pe.name LIKE \"%%%s%%\" ORDER BY pe.name COLLATE NOCASE",[[mSearchText stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {	
				playlist_entries_count++;
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		if (playlist_entries_count>MAX_SEARCH_RESULT) {
			tooMuchPL=playlist_entries_count;
			playlist_entries_count=MAX_SEARCH_RESULT;
		}
		
		if (playlist_entries_count) {
			playlist_entries=(t_playlist_entryS*)calloc(playlist_entries_count,sizeof(t_playlist_entryS));
			sprintf(sqlStatement,"SELECT p.name,p.id,pe.name,pe.fullpath FROM playlists_entries pe,playlists p \
					WHERE pe.id_playlist=p.id \
					AND pe.name LIKE \"%%%s%%\" ORDER BY pe.name COLLATE NOCASE",[[mSearchText stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {	
					playlist_entries[playlist_entries_idx].playlist_name=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
					playlist_entries[playlist_entries_idx].playlist_id=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
					playlist_entries[playlist_entries_idx].filename=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 2)];
					playlist_entries[playlist_entries_idx].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 3)];
															
					playlist_entries_idx++;
					if (playlist_entries_idx==playlist_entries_count) break;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		} else {
			playlist_entries=NULL;
		}
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return 0;
}

-(int) searchModland {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_idx;
	NSArray *mSearchTextArray=[mSearchText componentsSeparatedByString:@" "];
	
	
	if (db_entries_count) {
		for (int j=0;j<db_entries_count;j++) {
            db_entries[j].label=nil;
            db_entries[j].fullpath=nil;
		}
		free(db_entries);
		db_entries=NULL;
		db_entries_count=0;
	}
	pthread_mutex_lock(&db_mutex);
	db_entries_idx=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
				
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		db_entries_count=MAX_SEARCH_RESULT;
		if (db_entries_count) {
			db_entries=(t_db_browse_entryS*)calloc(db_entries_count,sizeof(t_db_browse_entryS));
			for (int j=0;j<db_entries_count;j++) {
				db_entries[j].id_mod=-1;
			}
			
			sprintf(sqlStatement,"SELECT filename,filesize,id,fullpath FROM mod_file \
					WHERE fullpath LIKE \"%%%s%%\"",[[(NSString*)[mSearchTextArray objectAtIndex:0] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
			for (int i=1;i<[mSearchTextArray count];i++) sprintf(sqlStatement,"%s AND fullpath LIKE \"%%%s%%\"",sqlStatement,[[[mSearchTextArray objectAtIndex:i] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
			sprintf(sqlStatement,"%s ORDER BY filename COLLATE NOCASE",sqlStatement);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			
			
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {	
					db_entries[db_entries_idx].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					db_entries[db_entries_idx].filesize=sqlite3_column_int(stmt, 1);
					db_entries[db_entries_idx].id_mod=sqlite3_column_int(stmt, 2);
					db_entries[db_entries_idx].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 3)];
					
					db_entries[db_entries_idx].downloaded=-1;

					db_entries_idx++;
					if (db_entries_idx==db_entries_count) break;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
			
			if (db_entries_idx<db_entries_count) db_entries_count=db_entries_idx;
			if (db_entries_count==0) {
				free(db_entries);
			}
			if (db_entries_count==MAX_SEARCH_RESULT) {
				sprintf(sqlStatement,"SELECT count(1) FROM mod_file \
						WHERE fullpath LIKE \"%%%s%%\"",[[(NSString*)[mSearchTextArray objectAtIndex:0] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
				for (int i=1;i<[mSearchTextArray count];i++) sprintf(sqlStatement,"%s AND fullpath LIKE \"%%%s%%\"",sqlStatement,[[[mSearchTextArray objectAtIndex:i] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
				err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
				if (err==SQLITE_OK){
					int nb_row;
					if (sqlite3_step(stmt) == SQLITE_ROW) {	
						nb_row=sqlite3_column_int(stmt, 0);
						if (nb_row>db_entries_count) tooMuchDB=nb_row;
					}
					sqlite3_finalize(stmt);
				} else NSLog(@"ErrSQL : %d",err);
				
			}
			
		} else {
			db_entries=NULL;
		}
		
		
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return 0;
}

-(int) searchASMA {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_idx;
	NSArray *mSearchTextArray=[mSearchText componentsSeparatedByString:@" "];
	
	if (dbASMA_entries_count) {
		for (int i=0;i<dbASMA_entries_count;i++) {
            dbASMA_entries[i].label=nil;
            dbASMA_entries[i].fullpath=nil;
            dbASMA_entries[i].id_md5=nil;
            dbASMA_entries[i].dir1=nil;
            dbASMA_entries[i].dir2=nil;
            dbASMA_entries[i].dir3=nil;
            dbASMA_entries[i].dir4=nil;
		}
		free(dbASMA_entries);
		dbASMA_entries=NULL;
		dbASMA_entries_count=0;
	}
	pthread_mutex_lock(&db_mutex);
	db_entries_idx=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
		
		dbASMA_entries_count=MAX_SEARCH_RESULT;
		if (dbASMA_entries_count) {
			dbASMA_entries=(t_dbASMA_browse_entryS*)calloc(dbASMA_entries_count,sizeof(t_dbASMA_browse_entryS));
			memset(dbASMA_entries,0,dbASMA_entries_count*sizeof(t_dbASMA_browse_entryS));
			
			sprintf(sqlStatement,"SELECT filename,id_md5,fullpath FROM asma_file \
					WHERE fullpath LIKE \"%%%s%%\"",[[[mSearchTextArray objectAtIndex:0] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
			for (int i=1;i<[mSearchTextArray count];i++) sprintf(sqlStatement,"%s AND fullpath LIKE \"%%%s%%\"",sqlStatement,[[[mSearchTextArray objectAtIndex:i] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
			sprintf(sqlStatement,"%s ORDER BY filename COLLATE NOCASE",sqlStatement);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {	
					dbASMA_entries[db_entries_idx].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					dbASMA_entries[db_entries_idx].id_md5=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
					dbASMA_entries[db_entries_idx].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 2)];
					dbASMA_entries[db_entries_idx].downloaded=-1;
					
					db_entries_idx++;
					if (db_entries_idx==dbASMA_entries_count) break;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
			
			if (db_entries_idx<dbASMA_entries_count) dbASMA_entries_count=db_entries_idx;
			if (dbASMA_entries_count==0) {
				free(dbASMA_entries);
			}
			if (dbASMA_entries_count==MAX_SEARCH_RESULT) {
				sprintf(sqlStatement,"SELECT count(1) FROM asma_file \
						WHERE fullpath LIKE \"%%%s%%\"",[[[mSearchTextArray objectAtIndex:0] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
				for (int i=1;i<[mSearchTextArray count];i++) sprintf(sqlStatement,"%s AND fullpath LIKE \"%%%s%%\"",sqlStatement,[[[mSearchTextArray objectAtIndex:i] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
				err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
				if (err==SQLITE_OK){
					int nb_row;
					if (sqlite3_step(stmt) == SQLITE_ROW) {	
						nb_row=sqlite3_column_int(stmt, 0);
						if (nb_row>dbASMA_entries_count) tooMuchDBASMA=nb_row;
					}
					sqlite3_finalize(stmt);
				} else NSLog(@"ErrSQL : %d",err);
				
			}
			
		} else {
			dbASMA_entries=NULL;
		}
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return 0;
}

-(int) searchHVSC {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_idx;
	NSArray *mSearchTextArray=[mSearchText componentsSeparatedByString:@" "];
	
	if (dbHVSC_entries_count) {
		for (int i=0;i<dbHVSC_entries_count;i++) {
            dbHVSC_entries[i].label=nil;
            dbHVSC_entries[i].fullpath=nil;
            dbHVSC_entries[i].id_md5=nil;
            dbHVSC_entries[i].dir1=nil;
            dbHVSC_entries[i].dir2=nil;
            dbHVSC_entries[i].dir3=nil;
            dbHVSC_entries[i].dir4=nil;
            dbHVSC_entries[i].dir5=nil;
		}
		free(dbHVSC_entries);
		dbHVSC_entries=NULL;
		dbHVSC_entries_count=0;
	}
	pthread_mutex_lock(&db_mutex);
	db_entries_idx=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		dbHVSC_entries_count=MAX_SEARCH_RESULT;
		if (dbHVSC_entries_count) {
			dbHVSC_entries=(t_dbHVSC_browse_entryS*)calloc(dbHVSC_entries_count,sizeof(t_dbHVSC_browse_entryS));
			memset(dbHVSC_entries,0,dbHVSC_entries_count*sizeof(t_dbHVSC_browse_entryS));
			
			sprintf(sqlStatement,"SELECT filename,id_md5,fullpath FROM hvsc_file \
					WHERE fullpath LIKE \"%%%s%%\"",[[[mSearchTextArray objectAtIndex:0] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
			for (int i=1;i<[mSearchTextArray count];i++) sprintf(sqlStatement,"%s AND fullpath LIKE \"%%%s%%\"",sqlStatement,[[[mSearchTextArray objectAtIndex:i] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
			sprintf(sqlStatement,"%s ORDER BY filename COLLATE NOCASE",sqlStatement);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					dbHVSC_entries[db_entries_idx].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[db_entries_idx].id_md5=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
					dbHVSC_entries[db_entries_idx].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 2)];
					dbHVSC_entries[db_entries_idx].downloaded=-1;
					
					db_entries_idx++;
					if (db_entries_idx==dbHVSC_entries_count) break;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
			
			if (db_entries_idx<dbHVSC_entries_count) dbHVSC_entries_count=db_entries_idx;
			if (dbHVSC_entries_count==0) {
				free(dbHVSC_entries);
			}
			if (dbHVSC_entries_count==MAX_SEARCH_RESULT) {
				sprintf(sqlStatement,"SELECT count(1) FROM hvsc_file \
						WHERE fullpath LIKE \"%%%s%%\"",[[[mSearchTextArray objectAtIndex:0] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
				for (int i=1;i<[mSearchTextArray count];i++) sprintf(sqlStatement,"%s AND fullpath LIKE \"%%%s%%\"",sqlStatement,[[[mSearchTextArray objectAtIndex:i] stringByReplacingOccurrencesOfString:@"*" withString:@"%"] UTF8String]);
				err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
				if (err==SQLITE_OK){
					int nb_row;
					if (sqlite3_step(stmt) == SQLITE_ROW) {
						nb_row=sqlite3_column_int(stmt, 0);
						if (nb_row>dbHVSC_entries_count) tooMuchDBHVSC=nb_row;
					}
					sqlite3_finalize(stmt);
				} else NSLog(@"ErrSQL : %d",err);
				
			}
			
		} else {
			dbHVSC_entries=NULL;
		}
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return 0;
}


-(int) searchLocal {
	NSString *file,*cpath;
	NSDirectoryEnumerator *dirEnum;
	NSDictionary *fileAttributes;
	NSMutableArray *dirToSearch;
	int prefix_length;
	int local_entries_idx;
	int dirMatched;
	NSRange r1,r2;
	int fileMatched;
	NSArray *mSearchTextArray=[mSearchText componentsSeparatedByString:@" "];
	
	if (local_entries_count) {
		for (int j=0;j<local_entries_count;j++) {
            local_entries[j].label=nil;
            local_entries[j].fullpath=nil;
		}
		free(local_entries);
		local_entries=NULL;
		local_entries_count=0;
	}
	
	local_entries_idx=0;
	dirToSearch = [[NSMutableArray alloc] init];
	
	local_entries_count=MAX_SEARCH_RESULT;
	local_entries=(t_local_browse_entryS*)calloc(local_entries_count,sizeof(t_local_browse_entryS));
	
	cpath=[NSHomeDirectory() stringByAppendingPathComponent: @"Documents"];
	prefix_length=[cpath length]+1;
	[dirToSearch addObject:cpath];
	while ([dirToSearch count]) {
		cpath=[dirToSearch objectAtIndex:0];
		[dirToSearch removeObjectAtIndex:0];
		
		dirMatched=0;
		for (int i=0;i<[mSearchTextArray count];i++) {
			r1.location=NSNotFound;
			r1 = [[cpath lastPathComponent] rangeOfString:[mSearchTextArray objectAtIndex:i] options:NSCaseInsensitiveSearch];
			if (r1.location != NSNotFound) dirMatched++;
		}
		if (dirMatched<[mSearchTextArray count]) dirMatched=0;
		
		dirEnum = [mFileMngr enumeratorAtPath:cpath];
		while (file = [dirEnum nextObject]) {
			fileAttributes=[dirEnum fileAttributes];
			if ([fileAttributes objectForKey:NSFileType]==NSFileTypeDirectory) {
				NSString *newDir=[NSString stringWithFormat:@"%@/%@",cpath,file];

				[dirToSearch addObject:newDir];
				[dirEnum skipDescendents];
			}
			if ([fileAttributes objectForKey:NSFileType]==NSFileTypeRegular) {
				//NSString *extension = [file pathExtension];
				NSString *file_no_ext = [file lastPathComponent];
				fileMatched=0;
				for (int i=0;i<[mSearchTextArray count];i++) {
					r2.location=NSNotFound;
					r2 = [file_no_ext rangeOfString:[mSearchTextArray objectAtIndex:i] options:NSCaseInsensitiveSearch];
					if (r2.location != NSNotFound) fileMatched++;
					else {
						r2 = [cpath rangeOfString:[mSearchTextArray objectAtIndex:i] options:NSCaseInsensitiveSearch];
						if (r2.location != NSNotFound) fileMatched++;
					}
				}
				if (fileMatched<[mSearchTextArray count]) fileMatched=0;
				if ((dirMatched||fileMatched))  {
					//found
					if (local_entries_idx<local_entries_count) {
						local_entries[local_entries_idx].label=[[NSString alloc] initWithString:file];
						if ([cpath length]==prefix_length-1) local_entries[local_entries_idx].fullpath=[[NSString alloc] initWithString:file];
						else local_entries[local_entries_idx].fullpath=[[NSString alloc] initWithFormat:@"%@/%@",[cpath substringFromIndex:prefix_length],file];
						//if (local_entries_idx==local_entries_count) break;
					}					
					local_entries_idx++;					
				}
			}
		}
	}
	if (local_entries_idx<local_entries_count) local_entries_count=local_entries_idx;
	if (local_entries_idx>local_entries_count) tooMuchLO=local_entries_idx;
	if (local_entries_count==0) free(local_entries);
	return 0;
}

-(void) showSearchPrgView {
    searchLabel.text=@"Searching...";
    searchPrgView.hidden=false;
}

-(void) hideSearchPrgView {
    searchPrgView.hidden=true;
}

-(void) searchThread {
	//NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    @autoreleasepool {
        mSearchProgress=0;
        if (playlist_searchOn&&(mSearchMode&1)) [self searchPlaylist];
        mSearchProgress=1;
        if (local_searchOn&&(mSearchMode&2)) [self searchLocal];
        mSearchProgress=2;
        if (modland_searchOn&&(mSearchMode&4)) [self searchModland];
        mSearchProgress=3;
        if (HVSC_searchOn&&(mSearchMode&8)) [self searchHVSC];
        mSearchProgress=4;
        if (ASMA_searchOn&&(mSearchMode&8)) [self searchASMA];
        //searchPrgView.hidden=YES;
        [self performSelectorOnMainThread:@selector(hideSearchPrgView) withObject:nil waitUntilDone:YES];
    }
	//[pool release];
}

-(void) updateSearchInfos: (NSTimer *) theTimer {
	if (searchPrgView.hidden==YES) {
		[theTimer invalidate];
		theTimer = nil; // ensures we never invalidate an already invalid Timer
		
		if (HVSC_searchOn+modland_searchOn+playlist_searchOn+local_searchOn==1) { //only 1 category selected => autoexpand
			if (modland_searchOn) modland_expanded=1;
			if (playlist_searchOn) playlist_expanded=1;
			if (local_searchOn) local_expanded=1;
			if (HVSC_searchOn) HVSC_expanded=1;
            if (ASMA_searchOn) ASMA_expanded=1;
		} else {
            if (ASMA_searchOn+HVSC_searchOn+modland_searchOn+playlist_searchOn+local_searchOn) {
                switch (lastSelectedSearch) {
                    case PLAYLIST_SEARCH:
                        playlist_expanded=1;
                        break;
                    case LOCAL_SEARCH:
                        local_expanded=1;
                        break;
                    case MODLAND_SEARCH:
                        modland_expanded=1;
                        break;
                    case HVSC_SEARCH:
                        HVSC_expanded=1;
                        break;
                    case ASMA_SEARCH:
                        ASMA_expanded=1;
                        break;
                }
            }
        }
		
		[tableView reloadData];
	} else {
		if (mSearchProgress==0) searchLabel.text=NSLocalizedString(@"Searching Playlists...",@"");
		if (mSearchProgress==1) searchLabel.text=NSLocalizedString(@"Searching Local files...",@"");
		if (mSearchProgress==2) searchLabel.text=NSLocalizedString(@"Searching MODLAND...",@"");
		if (mSearchProgress==3) searchLabel.text=NSLocalizedString(@"Searching HVSC...",@"");
        if (mSearchProgress==4) searchLabel.text=NSLocalizedString(@"Searching ASMA...",@"");
	}
}	

-(void) doSearch:(int)search_mode {
	if ([mSearchText length]<2) {
		UIAlertView *alertSearchMinChar = [[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Please enter at least 2 characters for your search.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
		[alertSearchMinChar show];

	} if (!(modland_searchOn+playlist_searchOn+local_searchOn+HVSC_searchOn+ASMA_searchOn)) {
		UIAlertView *alertSearchMinChar = [[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Please activate at least 1 section for your search.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
		[alertSearchMinChar show];
		
	} else {
		tooMuchDB=tooMuchDBHVSC=tooMuchDBASMA=tooMuchPL=tooMuchLO=0;
		//searchPrgView.hidden=NO;
        [self performSelectorOnMainThread:@selector(showSearchPrgView) withObject:nil waitUntilDone:YES];
		
		mSearchProgress=0;
		modland_expanded=local_expanded=playlist_expanded=HVSC_expanded=ASMA_expanded=0;
		mSearchMode=search_mode;
		[NSThread detachNewThreadSelector:@selector(searchThread) toTarget:self withObject:NULL];
		[NSTimer scheduledTimerWithTimeInterval: 0.2 target:self selector:@selector(updateSearchInfos:) userInfo:nil repeats: YES];
	}
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
	//if (mSearchText) [mSearchText release];
	mSearchText=[[NSString alloc] initWithString:searchText];
    
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
	//if (mSearchText) [mSearchText release];
	mSearchText=nil;
	sBar.text=nil;
	mSearch=0;
	sBar.showsCancelButton = NO;
	[searchBar resignFirstResponder];
}

- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar {
	[searchBar resignFirstResponder];
	[self doSearch:15];
//	[tableView reloadData];
}

#pragma mark -
#pragma mark Table view data source

- (void) headerPlaylistTapped: (UIButton*) sender {
	/* do what you want in response to section header tap */
	playlist_expanded^=1;
	[tableView reloadData];
//	NSIndexSet *_index=[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0,1)];
//	[tableView reloadSections:_index withRowAnimation: UITableViewRowAnimationFade];
}

- (void) headerLocalTapped: (UIButton*) sender {
	/* do what you want in response to section header tap */
	local_expanded^=1;
	[tableView reloadData];
//	NSIndexSet *_index=[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(1,1)];
//	[tableView reloadSections:_index withRowAnimation: UITableViewRowAnimationFade];
}

- (void) headerModlandTapped: (UIButton*) sender {
	/* do what you want in response to section header tap */
	modland_expanded^=1;
	[tableView reloadData];
//	NSIndexSet *_index=[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(2,1)];
//	[tableView reloadSections:_index withRowAnimation: UITableViewRowAnimationFade];
}

- (void) headerHVSCTapped: (UIButton*) sender {
	/* do what you want in response to section header tap */
	HVSC_expanded^=1;
	[tableView reloadData];
}

- (void) headerASMATapped: (UIButton*) sender {
	/* do what you want in response to section header tap */
	ASMA_expanded^=1;
	[tableView reloadData];
}

- (void) headerPlaylistTappedSearchOn: (UIButton*) sender {
	playlist_searchOn^=1;
	if (playlist_searchOn&&mSearchText) {
        lastSelectedSearch=PLAYLIST_SEARCH;
		if ([mSearchText length]>0) {
			[sBar resignFirstResponder];
			[self doSearch:1];
		}
	}
	if (!playlist_searchOn) {
		if (playlist_entries_count) {
			for (int j=0;j<playlist_entries_count;j++) {
                playlist_entries[j].playlist_name=nil;
                playlist_entries[j].playlist_id=nil;
                playlist_entries[j].filename=nil;
                playlist_entries[j].fullpath=nil;
			}
			playlist_entries_count=0;
		}
		playlist_expanded=0;
	}
	[tableView reloadData];
}

- (void) headerLocalTappedSearchOn: (UIButton*) sender {
	local_searchOn^=1;
	if (local_searchOn&&mSearchText) {
        lastSelectedSearch=LOCAL_SEARCH;
		if ([mSearchText length]>0) {
			[sBar resignFirstResponder];
			[self doSearch:2];
		}
	}
	if (!local_searchOn) {
		if (local_entries_count) {
			for (int j=0;j<local_entries_count;j++) {
				local_entries[j].label=nil;
				local_entries[j].fullpath=nil;
			}
			free(local_entries);
			local_entries=NULL;
			local_entries_count=0;
		}
		local_expanded=0;
	}
	[tableView reloadData];
}

- (void) headerModlandTappedSearchOn: (UIButton*) sender {
	modland_searchOn^=1;
	if (modland_searchOn&&mSearchText) {
        lastSelectedSearch=MODLAND_SEARCH;
		if ([mSearchText length]>0) {
			[sBar resignFirstResponder];
			[self doSearch:4];
		}
	}
	if (!modland_searchOn) {
		if (db_entries_count) {
			for (int j=0;j<db_entries_count;j++) {
                db_entries[j].label=nil;
                db_entries[j].fullpath=nil;
			}
			free(db_entries);
			db_entries=NULL;
			db_entries_count=0;
		}
		modland_expanded=0;
	}
	[tableView reloadData];
}

- (void) headerHVSCTappedSearchOn: (UIButton*) sender {
	HVSC_searchOn^=1;
	if (HVSC_searchOn&&mSearchText) {
        lastSelectedSearch=HVSC_SEARCH;
		if ([mSearchText length]>0) {
			[sBar resignFirstResponder];
			[self doSearch:8];
		}
	}
	if (!HVSC_searchOn) {
		if (dbHVSC_entries_count) {
			for (int i=0;i<dbHVSC_entries_count;i++) {
                dbHVSC_entries[i].label=nil;
                dbHVSC_entries[i].fullpath=nil;
                dbHVSC_entries[i].id_md5=nil;
                dbHVSC_entries[i].dir1=nil;
                dbHVSC_entries[i].dir2=nil;
                dbHVSC_entries[i].dir3=nil;
                dbHVSC_entries[i].dir4=nil;
                dbHVSC_entries[i].dir5=nil;
				
			}
			free(dbHVSC_entries);
			dbHVSC_entries=NULL;
			dbHVSC_entries_count=0;
		}
		HVSC_expanded=0;
	}
	[tableView reloadData];
}

- (void) headerASMATappedSearchOn: (UIButton*) sender {
	ASMA_searchOn^=1;
	if (ASMA_searchOn&&mSearchText) {
        lastSelectedSearch=ASMA_SEARCH;
		if ([mSearchText length]>0) {
			[sBar resignFirstResponder];
			[self doSearch:8];
		}
	}
	if (!ASMA_searchOn) {
		if (dbASMA_entries_count) {
			for (int i=0;i<dbASMA_entries_count;i++) {
                dbASMA_entries[i].label=nil;
                dbASMA_entries[i].fullpath=nil;
                dbASMA_entries[i].id_md5=nil;
                dbASMA_entries[i].dir1=nil;
                dbASMA_entries[i].dir2=nil;
                dbASMA_entries[i].dir3=nil;
                dbASMA_entries[i].dir4=nil;
			}
			free(dbASMA_entries);
			dbASMA_entries=NULL;
			dbASMA_entries_count=0;
		}
		ASMA_expanded=0;
	}
	[tableView reloadData];
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
        } else NSLog(@"ErrSQL : %d",err);
        
		sprintf(sqlStatement,"select localpath from mod_file where id=%d",id_mod);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				localpath=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);		
	}
	
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return localpath;
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
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		sprintf(sqlStatement,"SELECT localpath FROM mod_file WHERE id=%d",id_mod);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				strFullPath=[NSString stringWithUTF8String:(char*)sqlite3_column_text(stmt, 0)];
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
	}
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	
	checkPath = [documentsDirectory stringByAppendingPathComponent: [NSString stringWithFormat:@"%@/%@",MODLAND_BASEDIR,strFullPath]];
	success = [fileManager fileExistsAtPath:checkPath];
	if (success) return 1;
	return 0;
}

- (UIView*) tableView:(UITableView*)tableView viewForHeaderInSection: (NSInteger) section {
	UIView *customView = [[UIView alloc] initWithFrame: CGRectMake(0.0, 0.0, tableView.bounds.size.width, 32.0)];
	

	CALayer *layerU = [CALayer layer];
	layerU.frame = CGRectMake(0.0, 0.0, tableView.bounds.size.width, 1.0);
    
	[customView.layer insertSublayer:layerU atIndex:0];
	
	CAGradientLayer *gradient = [CAGradientLayer layer];
	gradient.frame = CGRectMake(0.0, 1.0, tableView.bounds.size.width, 30.0);
    
	[customView.layer insertSublayer:gradient atIndex:0];
	
	CALayer *layerD = [CALayer layer];
	layerD.frame = CGRectMake(0.0, 31.0, tableView.bounds.size.width, 1.0);
    
	[customView.layer insertSublayer:layerD atIndex:0];
    
    if (darkMode) {
        customView.backgroundColor = [UIColor colorWithRed: 1-0.7f green: 1-0.7f blue: 1-0.7f alpha: 1.0f];
        layerU.backgroundColor = [[UIColor colorWithRed: 1-183.0f/255.0f green: 1-193.0f/255.0f blue: 1-199.0f/255.0f alpha: 1.00] CGColor];
        gradient.colors = [NSArray arrayWithObjects:(id)[[UIColor colorWithRed: 1-144.0f/255.0f green: 1-159.0f/255.0f blue: 1-177.0f/255.0f alpha: 1.00] CGColor],
                           (id)[[UIColor colorWithRed: 1-183.0f/255.0f green: 1-193.0f/255.0f blue: 1-199.0f/255.0f  alpha: 1.00] CGColor], nil];
        layerD.backgroundColor = [[UIColor colorWithRed: 1-144.0f/255.0f green: 1-159.0f/255.0f blue: 1-177.0f/255.0f alpha: 1.00] CGColor];
    } else {
        customView.backgroundColor = [UIColor colorWithRed: 0.7f green: 0.7f blue: 0.7f alpha: 1.0f];
        layerU.backgroundColor = [[UIColor colorWithRed: 183.0f/255.0f green: 193.0f/255.0f blue: 199.0f/255.0f alpha: 1.00] CGColor];
        gradient.colors = [NSArray arrayWithObjects:(id)[[UIColor colorWithRed: 144.0f/255.0f green: 159.0f/255.0f blue: 177.0f/255.0f alpha: 1.00] CGColor],
                           (id)[[UIColor colorWithRed: 183.0f/255.0f green: 193.0f/255.0f blue: 199.0f/255.0f  alpha: 1.00] CGColor], nil];
        layerD.backgroundColor = [[UIColor colorWithRed: 144.0f/255.0f green: 159.0f/255.0f blue: 177.0f/255.0f alpha: 1.00] CGColor];
    }
	
	UIButton *buttonLeft = [[UIButton alloc] initWithFrame: CGRectMake(0.0, 0.0, 32, 32)];
	UIButton *buttonRight = [[UIButton alloc] initWithFrame: CGRectMake(tableView.bounds.size.width-32, 0.0, 32, 32)];
	// Prepare target-action
	
	UIButton *buttonLabel                  = [UIButton buttonWithType: UIButtonTypeCustom];
	buttonLabel.titleLabel.font            = [UIFont boldSystemFontOfSize: 20];
	buttonLabel.titleLabel.shadowOffset    = CGSizeMake (0.0, 1.0);
	buttonLabel.titleLabel.lineBreakMode   = (NSLineBreakMode)UILineBreakModeTailTruncation;
//	buttonLabel.titleLabel.shadowOffset    = CGSizeMake (1.0, 0.0);
	buttonLabel.frame=CGRectMake(32, 0.0, tableView.bounds.size.width-32*2, 32);
	
	
	switch (section) {
		case 0:
			if (playlist_searchOn&&playlist_entries_count) {
			if (playlist_expanded) [buttonRight setImage:[UIImage imageNamed:@"expanded.png"]  forState: UIControlStateNormal];
			else [buttonRight setImage:[UIImage imageNamed:@"collapsed.png"]  forState: UIControlStateNormal];			
			} else [buttonRight setImage:nil  forState: UIControlStateNormal];			
			[buttonRight addTarget: self action: @selector(headerPlaylistTapped:) forControlEvents: UIControlEventTouchUpInside];
			
			if (playlist_entries_count) {
				//[customView addSubview: buttonRight];			
				[buttonLabel addTarget: self action: @selector(headerPlaylistTapped:) forControlEvents: UIControlEventTouchUpInside];
			} else [buttonLabel addTarget: self action: @selector(headerPlaylistTappedSearchOn:) forControlEvents: UIControlEventTouchUpInside];
			
			if (playlist_searchOn) {
				[buttonLeft setImage:[UIImage imageNamed:@"checkbox_single.png"]  forState: UIControlStateNormal];
				if (tooMuchPL) {
					[buttonLabel setTitle:[NSString stringWithFormat:@"Playlist (%d, limited to %d)",tooMuchPL,playlist_entries_count] forState:UIControlStateNormal];
					buttonLabel.titleLabel.font            = [UIFont boldSystemFontOfSize: 16];
				} else [buttonLabel setTitle:[NSString stringWithFormat:NSLocalizedString(@"Playlist (%d)",@""),playlist_entries_count] forState:UIControlStateNormal];
				if (darkMode) buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1-1.0f green:1-1.0f blue:1-1.0f alpha:1.0f];
                else buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:1.0f];
			} else  {
				[buttonLeft setImage:[UIImage imageNamed:@"checkbox_unchecked.png"]  forState: UIControlStateNormal];
				[buttonLabel setTitle:[NSString stringWithString:NSLocalizedString(@"Playlist/Search off",@"")] forState:UIControlStateNormal];
				if (darkMode) buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1-0.9f green:1-0.9f blue:1-0.9f alpha:1.0f];
                else buttonLabel.titleLabel.textColor = [UIColor colorWithRed:0.9f green:0.9f blue:0.9f alpha:1.0f];
			}
			[buttonLeft addTarget: self action: @selector(headerPlaylistTappedSearchOn:) forControlEvents: UIControlEventTouchUpInside];			
			break;
		case 1:
			if (local_searchOn&&local_entries_count) {
			if (local_expanded) [buttonRight setImage:[UIImage imageNamed:@"expanded.png"]  forState: UIControlStateNormal];
			else [buttonRight setImage:[UIImage imageNamed:@"collapsed.png"]  forState: UIControlStateNormal];
			} else [buttonRight setImage:nil  forState: UIControlStateNormal];			
			[buttonRight addTarget: self action: @selector(headerLocalTapped:) forControlEvents: UIControlEventTouchUpInside];
			
			if (local_entries_count) {
				//[customView addSubview: buttonRight];
				[buttonLabel addTarget: self action: @selector(headerLocalTapped:) forControlEvents: UIControlEventTouchUpInside];
			} else [buttonLabel addTarget: self action: @selector(headerLocalTappedSearchOn:) forControlEvents: UIControlEventTouchUpInside];

			if (local_searchOn) {
				[buttonLeft setImage:[UIImage imageNamed:@"checkbox_single.png"]  forState: UIControlStateNormal];
				if (tooMuchLO) {
					[buttonLabel setTitle:[NSString stringWithFormat:NSLocalizedString(@"Local (%d, limited to %d)",@""),tooMuchLO,local_entries_count] forState:UIControlStateNormal];
					buttonLabel.titleLabel.font            = [UIFont boldSystemFontOfSize: 16];
				} else [buttonLabel setTitle:[NSString stringWithFormat:NSLocalizedString(@"Local (%d)",@""),local_entries_count] forState:UIControlStateNormal];
				if (darkMode) buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1-1.0f green:1-1.0f blue:1-1.0f alpha:1.0f];
                else buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:1.0f];
			} else {
				[buttonLeft setImage:[UIImage imageNamed:@"checkbox_unchecked.png"]  forState: UIControlStateNormal];
				[buttonLabel setTitle:[NSString stringWithString:NSLocalizedString(@"Local/Search off",@"")] forState:UIControlStateNormal];
				if (darkMode) buttonLabel.titleLabel.textColor = [UIColor colorWithRed:0.9f green:0.9f blue:0.9f alpha:1.0f];
                else buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1-0.9f green:1-0.9f blue:1-0.9f alpha:1.0f];
			}
			[buttonLeft addTarget: self action: @selector(headerLocalTappedSearchOn:) forControlEvents: UIControlEventTouchUpInside];			
			break;
		case 2:
			if (modland_searchOn&&db_entries_count) {
			if (modland_expanded) [buttonRight setImage:[UIImage imageNamed:@"expanded.png"]  forState: UIControlStateNormal];
			else [buttonRight setImage:[UIImage imageNamed:@"collapsed.png"]  forState: UIControlStateNormal];
			[buttonRight addTarget: self action: @selector(headerModlandTapped:) forControlEvents: UIControlEventTouchUpInside];
			} else [buttonRight setImage:nil  forState: UIControlStateNormal];
			if (db_entries_count) {
				//[customView addSubview: buttonRight];
				[buttonLabel addTarget: self action: @selector(headerModlandTapped:) forControlEvents: UIControlEventTouchUpInside];
			} else [buttonLabel addTarget: self action: @selector(headerModlandTappedSearchOn:) forControlEvents: UIControlEventTouchUpInside];
												
			if (modland_searchOn) {
				[buttonLeft setImage:[UIImage imageNamed:@"checkbox_single.png"]  forState: UIControlStateNormal];
				if (tooMuchDB) {
					[buttonLabel setTitle:[NSString stringWithFormat:NSLocalizedString(@"Modland (%d, limited to %d)",@""),tooMuchDB,db_entries_count] forState:UIControlStateNormal];
					buttonLabel.titleLabel.font            = [UIFont boldSystemFontOfSize: 16];
				} else [buttonLabel setTitle:[NSString stringWithFormat:NSLocalizedString(@"Modland (%d)",@""),db_entries_count] forState:UIControlStateNormal];
				if (darkMode) buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1-1.0f green:1-1.0f blue:1-1.0f alpha:1.0f];
                else buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:1.0f];
			} else {
				[buttonLeft setImage:[UIImage imageNamed:@"checkbox_unchecked.png"]  forState: UIControlStateNormal];
				[buttonLabel setTitle:[NSString stringWithString:NSLocalizedString(@"Modland/Search off",@"")] forState:UIControlStateNormal];
				if (darkMode) buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1-0.9f green:1-0.9f blue:1-0.9f alpha:1.0f];
                else buttonLabel.titleLabel.textColor = [UIColor colorWithRed:0.9f green:0.9f blue:0.9f alpha:1.0f];
			}
			[buttonLeft addTarget: self action: @selector(headerModlandTappedSearchOn:) forControlEvents: UIControlEventTouchUpInside];			
			break;
		case 3:
			if (HVSC_searchOn&&dbHVSC_entries_count) {
			if (HVSC_expanded) [buttonRight setImage:[UIImage imageNamed:@"expanded.png"]  forState: UIControlStateNormal];
			else [buttonRight setImage:[UIImage imageNamed:@"collapsed.png"]  forState: UIControlStateNormal];
			[buttonRight addTarget: self action: @selector(headerHVSCTapped:) forControlEvents: UIControlEventTouchUpInside];
			} else [buttonRight setImage:nil  forState: UIControlStateNormal];
			if (dbHVSC_entries_count) {
				//[customView addSubview: buttonRight];
				[buttonLabel addTarget: self action: @selector(headerHVSCTapped:) forControlEvents: UIControlEventTouchUpInside];
			} else [buttonLabel addTarget: self action: @selector(headerHVSCTappedSearchOn:) forControlEvents: UIControlEventTouchUpInside];
			
			if (HVSC_searchOn) {
				[buttonLeft setImage:[UIImage imageNamed:@"checkbox_single.png"]  forState: UIControlStateNormal];
				if (tooMuchDBHVSC) {
					[buttonLabel setTitle:[NSString stringWithFormat:NSLocalizedString(@"HVSC (%d, limited to %d)",@""),tooMuchDBHVSC,dbHVSC_entries_count] forState:UIControlStateNormal];
					buttonLabel.titleLabel.font            = [UIFont boldSystemFontOfSize: 16];
				} else [buttonLabel setTitle:[NSString stringWithFormat:NSLocalizedString(@"HVSC (%d)",@""),dbHVSC_entries_count] forState:UIControlStateNormal];
				if (darkMode) buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:1.0f];
                else buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1-1.0f green:1-1.0f blue:1-1.0f alpha:1.0f];
			} else {
				[buttonLeft setImage:[UIImage imageNamed:@"checkbox_unchecked.png"]  forState: UIControlStateNormal];
				[buttonLabel setTitle:[NSString stringWithString:NSLocalizedString(@"HVSC/Search off",@"")] forState:UIControlStateNormal];
				if (darkMode) buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1-0.9f green:1-0.9f blue:1-0.9f alpha:1.0f];
                else buttonLabel.titleLabel.textColor = [UIColor colorWithRed:0.9f green:0.9f blue:0.9f alpha:1.0f];
			}
			[buttonLeft addTarget: self action: @selector(headerHVSCTappedSearchOn:) forControlEvents: UIControlEventTouchUpInside];			
			break;
        case 4:
			if (ASMA_searchOn&&dbASMA_entries_count) {
                if (ASMA_expanded) [buttonRight setImage:[UIImage imageNamed:@"expanded.png"]  forState: UIControlStateNormal];
                else [buttonRight setImage:[UIImage imageNamed:@"collapsed.png"]  forState: UIControlStateNormal];
                [buttonRight addTarget: self action: @selector(headerASMATapped:) forControlEvents: UIControlEventTouchUpInside];
			} else [buttonRight setImage:nil  forState: UIControlStateNormal];
			if (dbASMA_entries_count) {
				//[customView addSubview: buttonRight];
				[buttonLabel addTarget: self action: @selector(headerASMATapped:) forControlEvents: UIControlEventTouchUpInside];
			} else [buttonLabel addTarget: self action: @selector(headerASMATappedSearchOn:) forControlEvents: UIControlEventTouchUpInside];
			
			if (ASMA_searchOn) {
				[buttonLeft setImage:[UIImage imageNamed:@"checkbox_single.png"]  forState: UIControlStateNormal];
				if (tooMuchDBASMA) {
					[buttonLabel setTitle:[NSString stringWithFormat:NSLocalizedString(@"ASMA (%d, limited to %d)",@""),tooMuchDBASMA,dbASMA_entries_count] forState:UIControlStateNormal];
					buttonLabel.titleLabel.font            = [UIFont boldSystemFontOfSize: 16];
				} else [buttonLabel setTitle:[NSString stringWithFormat:NSLocalizedString(@"ASMA (%d)",@""),dbASMA_entries_count] forState:UIControlStateNormal];
				if (darkMode) buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1-1.0f green:1-1.0f blue:1-1.0f alpha:1.0f];
                else buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:1.0f];
			} else {
				[buttonLeft setImage:[UIImage imageNamed:@"checkbox_unchecked.png"]  forState: UIControlStateNormal];
				[buttonLabel setTitle:[NSString stringWithString:NSLocalizedString(@"ASMA/Search off",@"")] forState:UIControlStateNormal];
				if (darkMode) buttonLabel.titleLabel.textColor = [UIColor colorWithRed:1-0.9f green:1-0.9f blue:1-0.9f alpha:1.0f];
                else buttonLabel.titleLabel.textColor = [UIColor colorWithRed:0.9f green:0.9f blue:0.9f alpha:1.0f];
			}
			[buttonLeft addTarget: self action: @selector(headerASMATappedSearchOn:) forControlEvents: UIControlEventTouchUpInside];
			break;
	}
	
	[customView addSubview: buttonLeft];
	[customView addSubview: buttonLabel];
	[customView addSubview: buttonRight];
	
	return customView;
}

-(void)tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath {
	//get real index of cell 
	local_flag=0;
	for (int i=0;i<indexPath.section;i++) {
		local_flag+=[tableView numberOfRowsInSection:i];
	}
	local_flag+=indexPath.row;
	if (local_flag&1) [cell setBackgroundColor:[UIColor colorWithRed:.95f green:.95f blue:.95f alpha:1]];
	else [cell setBackgroundColor:[UIColor clearColor]];	
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
	return @"";
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	local_flag=0;
	return 5;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if (section==0) {
		if (playlist_expanded==0) return 0;
		return playlist_entries_count;
	} else if (section==1) {
		if (local_expanded==0) return 0;
		return local_entries_count;
	} else if (section==2) {	
		if (modland_expanded==0) return 0;
		return db_entries_count;
	} else if (section==3) {	
		if (HVSC_expanded==0) return 0;
		return dbHVSC_entries_count;
	} else if (section==4) {
		if (ASMA_expanded==0) return 0;
		return dbASMA_entries_count;
	}
	return 0;
}

-(void) refreshViewAfterDownload {
	NSFileManager *fileManager = mFileMngr;
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];
	NSString *checkPath;
	for (int i=0;i<db_entries_count;i++) 
		if (db_entries[i].downloaded==0) {
			checkPath = [documentsDirectory stringByAppendingPathComponent:[NSString stringWithFormat:@"%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:db_entries[i].id_mod]]];
			if ([fileManager fileExistsAtPath:checkPath]) db_entries[i].downloaded=1;
		}
	for (int i=0;i<dbHVSC_entries_count;i++) 
		if (dbHVSC_entries[i].downloaded==0) {
			checkPath = [documentsDirectory stringByAppendingPathComponent:[NSString stringWithFormat:@"%@%@",HVSC_BASEDIR,dbHVSC_entries[i].fullpath]];
			if ([fileManager fileExistsAtPath:checkPath]) dbHVSC_entries[i].downloaded=1;
		}
    for (int i=0;i<dbASMA_entries_count;i++)
		if (dbASMA_entries[i].downloaded==0) {
			checkPath = [documentsDirectory stringByAppendingPathComponent:[NSString stringWithFormat:@"%@%@",ASMA_BASEDIR,dbASMA_entries[i].fullpath]];
			if ([fileManager fileExistsAtPath:checkPath]) dbASMA_entries[i].downloaded=1;
		}
	[tableView reloadData];
}

-(void) loadPlayListsFromDB:(NSString *)_id_playlist intoPlaylist:(t_playlistS *)_playlist  {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
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
		
		//Get playlist entries
		_playlist->nb_entries=0;
		sprintf(sqlStatement,"SELECT name,fullpath FROM playlists_entries WHERE id_playlist=%s",[_id_playlist UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				_playlist->label[_playlist->nb_entries]=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
				_playlist->fullpath[_playlist->nb_entries]=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
				_playlist->nb_entries++;
				//TODO : add check / max size (1024)
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}

-(void) doPrimAction:(NSIndexPath *)indexPath {
	if (indexPath.section==0) {//playlist
        playlist=(t_playlistS*)calloc(1,sizeof(t_playlistS));
        [self loadPlayListsFromDB:playlist_entries[indexPath.row].playlist_id intoPlaylist:playlist];
		int pos=0,flag=0;
		
		NSMutableArray *array_label = [[NSMutableArray alloc] init];
		NSMutableArray *array_path = [[NSMutableArray alloc] init];
		for (int j=0;j<playlist->nb_entries;j++) {
			[array_label addObject:playlist->label[j]];
			[array_path addObject:playlist->fullpath[j]];
			if ([playlist->fullpath[j] compare:playlist_entries[indexPath.row].fullpath]==NSOrderedSame) flag=1;
			if (flag==0) pos++;
		}

		[detailViewController play_listmodules:array_label start_index:pos path:array_path];
		
        free(playlist);
	}
	if (indexPath.section==1) {//local files
		NSMutableArray *array_label = [[NSMutableArray alloc] init];
		NSMutableArray *array_path = [[NSMutableArray alloc] init];
		[array_label addObject:local_entries[indexPath.row].label];
		[array_path addObject:[NSString stringWithFormat:@"Documents/%@",local_entries[indexPath.row].fullpath]];
		[detailViewController play_listmodules:array_label start_index:0 path:array_path];
		
	}
	if (indexPath.section==2) {//MODLAND
		NSString *filePath=db_entries[indexPath.row].fullpath;
		NSString *modFilename=db_entries[indexPath.row].label;
		NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
		NSString *localPath=[NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:db_entries[indexPath.row].id_mod]];
		
		if (db_entries[indexPath.row].downloaded==1) {  //file already downloaded
			NSMutableArray *array_label = [[NSMutableArray alloc] init];
			NSMutableArray *array_path = [[NSMutableArray alloc] init];
			[array_label addObject:db_entries[indexPath.row].label];
			[array_path addObject:localPath];
			[detailViewController play_listmodules:array_label start_index:0 path:array_path];
			
		} else {		//download
			NSString *completePath=[NSString stringWithFormat:@"%@",[NSHomeDirectory() stringByAppendingPathComponent:  [localPath stringByDeletingLastPathComponent]]];
			NSError *err;
			[mFileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:&err];
            
            NSString *modland_url=[NSString stringWithFormat:@"%s",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text];
            NSRange nsr=[modland_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
            if (nsr.location==NSNotFound) {
                //HTTP
                [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:db_entries[indexPath.row].filesize isMODLAND:1 usePrimaryAction:1];
                
            } else {
                //FTP
                [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:db_entries[indexPath.row].filesize filename:modFilename isMODLAND:1 usePrimaryAction:1];
            }
            
			//[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:MODLAND_FTPHOST filesize:db_entries[indexPath.row].filesize filename:modFilename isMODLAND:1 usePrimaryAction:1];
		}
	}
	if (indexPath.section==3) {//HVSC
		//File selected, start download is needed
		NSString *sidFilename=[NSString stringWithFormat:@"%@",dbHVSC_entries[indexPath.row].label];
		NSString *ftpPath=[NSString stringWithFormat:@"%@",dbHVSC_entries[indexPath.row].fullpath];				
		NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,dbHVSC_entries[indexPath.row].fullpath];
		
		if (dbHVSC_entries[indexPath.row].downloaded==1) {
			NSMutableArray *array_label = [[NSMutableArray alloc] init];
			NSMutableArray *array_path = [[NSMutableArray alloc] init ];
			[array_label addObject:sidFilename];
			[array_path addObject:localPath];
			[detailViewController play_listmodules:array_label start_index:0 path:array_path];
			
		} else {
			NSString *completePath=[NSString stringWithFormat:@"%@/%@%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],HVSC_BASEDIR,[dbHVSC_entries[indexPath.row].fullpath stringByDeletingLastPathComponent]];
			NSError *err;
			[mFileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:&err];
			
			//[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:HVSC_FTPHOST filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:1];
            //[downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",HVSC_HTTPHOST,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:1];
            
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
    if (indexPath.section==4) {//ASMA
		//File selected, start download is needed
		NSString *sidFilename=[NSString stringWithFormat:@"%@",dbASMA_entries[indexPath.row].label];
		NSString *ftpPath=[NSString stringWithFormat:@"%@",dbASMA_entries[indexPath.row].fullpath];
		NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",ASMA_BASEDIR,dbASMA_entries[indexPath.row].fullpath];
		
		if (dbASMA_entries[indexPath.row].downloaded==1) {
			NSMutableArray *array_label = [[NSMutableArray alloc] init];
			NSMutableArray *array_path = [[NSMutableArray alloc] init];
			[array_label addObject:sidFilename];
			[array_path addObject:localPath];
			[detailViewController play_listmodules:array_label start_index:0 path:array_path];
			
		} else {
			NSString *completePath=[NSString stringWithFormat:@"%@/%@%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],ASMA_BASEDIR,[dbASMA_entries[indexPath.row].fullpath stringByDeletingLastPathComponent]];
			NSError *err;
			[mFileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:&err];
			
			NSString *asma_url=[NSString stringWithFormat:@"%s",settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text];
            NSRange nsr=[asma_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
            if (nsr.location==NSNotFound) {
                //HTTP
                [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",asma_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:1];
            } else {
                //FTP
                [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[asma_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:1];
            }
		}
	}
}

-(void) doSecAction:(NSIndexPath *)indexPath {
	if (indexPath.section==0) {//playlist
		if ([detailViewController add_to_playlist:playlist_entries[indexPath.row].fullpath fileName:playlist_entries[indexPath.row].filename forcenoplay:1]) {
			
		}
	}
	
	if (indexPath.section==1) {//local files
		if ([detailViewController add_to_playlist:[NSString stringWithFormat:@"Documents/%@",local_entries[indexPath.row].fullpath] fileName:local_entries[indexPath.row].label forcenoplay:1]) {
			
		}
	}
	if (indexPath.section==2) {//MODLAND
		NSString *filePath=db_entries[indexPath.row].fullpath;
		NSString *modFilename=db_entries[indexPath.row].label;
		NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
		NSString *localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:db_entries[indexPath.row].id_mod]];
		
		if (db_entries[indexPath.row].downloaded==1) { //file already downloaded
			if ([detailViewController add_to_playlist:localPath fileName:db_entries[indexPath.row].label forcenoplay:1]) {
				
			}
		} else {  //download
			NSString *completePath=[NSString stringWithFormat:@"%@",[NSHomeDirectory() stringByAppendingPathComponent:  [localPath stringByDeletingLastPathComponent]]];
			NSError *err;
			[mFileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:&err];
            
            NSString *modland_url=[NSString stringWithFormat:@"%s",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text];
            NSRange nsr=[modland_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
            if (nsr.location==NSNotFound) {
                //HTTP
                [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:db_entries[indexPath.row].filesize isMODLAND:1 usePrimaryAction:0];
                
            } else {
                //FTP
                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:db_entries[indexPath.row].filesize filename:modFilename isMODLAND:1 usePrimaryAction:0];
            }
            
			
		}
	}
	if (indexPath.section==3) {//HVSC
		//File selected, start download is needed
		NSString *sidFilename=[NSString stringWithFormat:@"%@",dbHVSC_entries[indexPath.row].label];
		NSString *ftpPath=[NSString stringWithFormat:@"%@",dbHVSC_entries[indexPath.row].fullpath];				
		NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,dbHVSC_entries[indexPath.row].fullpath];
		
		if (dbHVSC_entries[indexPath.row].downloaded==1) {
			if ([detailViewController add_to_playlist:localPath fileName:dbHVSC_entries[indexPath.row].label forcenoplay:1]) {
				
			}
		} else {
			NSString *completePath=[NSString stringWithFormat:@"%@/%@%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],HVSC_BASEDIR,[dbHVSC_entries[indexPath.row].fullpath stringByDeletingLastPathComponent]];
			NSError *err;
			[mFileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:&err];
			
			//[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:HVSC_FTPHOST filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:0];
            //[downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",HVSC_HTTPHOST,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:0];
            
            NSString *hvsc_url=[NSString stringWithFormat:@"%s",settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text];
            NSRange nsr=[hvsc_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
            if (nsr.location==NSNotFound) {
                //HTTP
                [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",hvsc_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:0];
            } else {
                //FTP
                [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[hvsc_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:0];
            }
		}
	}
    if (indexPath.section==4) {//ASMA
		//File selected, start download is needed
		NSString *sidFilename=[NSString stringWithFormat:@"%@",dbASMA_entries[indexPath.row].label];
		NSString *ftpPath=[NSString stringWithFormat:@"%@",dbASMA_entries[indexPath.row].fullpath];
		NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",ASMA_BASEDIR,dbASMA_entries[indexPath.row].fullpath];
		
		if (dbASMA_entries[indexPath.row].downloaded==1) {
			if ([detailViewController add_to_playlist:localPath fileName:dbASMA_entries[indexPath.row].label forcenoplay:1]) {
				
			}
		} else {
			NSString *completePath=[NSString stringWithFormat:@"%@/%@%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],ASMA_BASEDIR,[dbASMA_entries[indexPath.row].fullpath stringByDeletingLastPathComponent]];
			NSError *err;
			[mFileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:&err];
			
            NSString *asma_url=[NSString stringWithFormat:@"%s",settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text];
            NSRange nsr=[asma_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
            if (nsr.location==NSNotFound) {
                //HTTP
                [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",asma_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:0];
            } else {
                //FTP
                [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[asma_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:0];
            }
		}
	}
}

- (void) primaryActionTapped: (UIButton*) sender {
	NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
	[tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
	[self doPrimAction:indexPath];
}

- (void) secondaryActionTapped: (UIButton*) sender {
	NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
	[tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
	[self doSecAction:indexPath];
}

// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {

}	
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
	// Return NO if you do not want the item to be re-orderable.
	return NO;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"Cell";
	const NSInteger TOP_LABEL_TAG = 1001;
	const NSInteger BOTTOM_LABEL_TAG = 1002;
	const NSInteger ACT_IMAGE_TAG = 1003;
	UILabel *topLabel;
	UILabel *bottomLabel;
	UIButton *actionView;

    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if ((cell == nil)||forceReloadCells) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
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
        topLabel.font = [UIFont boldSystemFontOfSize:18];
        topLabel.lineBreakMode=(NSLineBreakMode)UILineBreakModeMiddleTruncation;
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
        bottomLabel.lineBreakMode=(NSLineBreakMode)UILineBreakModeMiddleTruncation;
        bottomLabel.opaque=TRUE;

		actionView = [UIButton buttonWithType: UIButtonTypeCustom];		
		[cell.contentView addSubview:actionView];
		actionView.tag = ACT_IMAGE_TAG;
		
		if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value>=1) {
			[actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
		} else {
			[actionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
		}
		
	} else {
		topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
		bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
		actionView = (UIButton *)[cell viewWithTag:ACT_IMAGE_TAG];		
    }
	bottomLabel.text=@""; //default value
	cell.accessoryType=UITableViewCellAccessoryNone;
	cell.tag=indexPath.row;
	//cell.selectionStyle= UITableViewCellSelectionStyleGray;

	if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value>=1) {
		[actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateNormal];
		[actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateHighlighted];
	} else {
		[actionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateNormal];
		[actionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateHighlighted];
	}
	
	actionView.frame = CGRectMake(tableView.bounds.size.width - 40-tableView.safeAreaInsets.left-tableView.safeAreaInsets.right,0,40,40);
	
	bottomLabel.frame = CGRectMake( cell.indentationWidth*1.0,
									22,
									tableView.bounds.size.width - cell.indentationWidth*1.0 - 40,
								   18);
	topLabel.frame = CGRectMake( cell.indentationWidth*1.0,
								0,
								tableView.bounds.size.width - cell.indentationWidth*1.0-40,
								22);

    if (darkMode) {
        topLabel.textColor=[UIColor colorWithRed:1-0.1f green:1-0.1f blue:1-0.1f alpha:1.0f];
        topLabel.highlightedTextColor=[UIColor colorWithRed:1-0.9f green:1-0.9f blue:1-0.9f alpha:1.0f];
        bottomLabel.textColor = [UIColor colorWithRed:1-0.4 green:1-0.4 blue:1-0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:1-0.8 green:1-0.8 blue:1-0.8 alpha:1.0];
    } else {
        topLabel.textColor=[UIColor colorWithRed:0.1f green:0.1f blue:0.1f alpha:1.0f];
        topLabel.highlightedTextColor=[UIColor colorWithRed:0.9f green:0.9f blue:0.9f alpha:1.0f];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
    }

    
	// Set up the cell...
	if (indexPath.section==0) {
		topLabel.text=playlist_entries[indexPath.row].filename;
		bottomLabel.text = [NSString stringWithFormat:@"%@", playlist_entries[indexPath.row].playlist_name];
	}
	if (indexPath.section==1) {
		topLabel.text=local_entries[indexPath.row].label;
		bottomLabel.text = [NSString stringWithFormat:@"%@", local_entries[indexPath.row].fullpath];
			}
	if (indexPath.section==2) {
		topLabel.text=[NSString stringWithFormat:@"%@/%dKB",db_entries[indexPath.row].label,db_entries[indexPath.row].filesize/1024];
		if (db_entries[indexPath.row].downloaded==-1) {
			db_entries[indexPath.row].downloaded=[self checkIsDownloadedMod:db_entries[indexPath.row].id_mod];
		}
		if (db_entries[indexPath.row].downloaded==0) {
            topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0f];
        }
		bottomLabel.text = db_entries[indexPath.row].fullpath;						
	}	
	if (indexPath.section==3) {
		topLabel.text=[NSString stringWithFormat:@"%@",dbHVSC_entries[indexPath.row].label];
		
		if (dbHVSC_entries[indexPath.row].downloaded==-1) {
			NSFileManager *fileManager = mFileMngr;
			BOOL success;
			NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
			NSString *documentsDirectory = [paths objectAtIndex:0];
			NSString *checkPath;
			
			checkPath = [documentsDirectory stringByAppendingPathComponent:[NSString stringWithFormat:@"%@%@",HVSC_BASEDIR,dbHVSC_entries[indexPath.row].fullpath]];
			success = [fileManager fileExistsAtPath:checkPath];
			if (success) dbHVSC_entries[indexPath.row].downloaded=1;
			else dbHVSC_entries[indexPath.row].downloaded=0;
		}
		if (dbHVSC_entries[indexPath.row].downloaded==0) {
            topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0f];
        }
		bottomLabel.text = dbHVSC_entries[indexPath.row].fullpath;
	}
    if (indexPath.section==4) {
		topLabel.text=[NSString stringWithFormat:@"%@",dbASMA_entries[indexPath.row].label];
		
		if (dbASMA_entries[indexPath.row].downloaded==-1) {
			NSFileManager *fileManager = mFileMngr;
			BOOL success;
			NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
			NSString *documentsDirectory = [paths objectAtIndex:0];
			NSString *checkPath;
			
			checkPath = [documentsDirectory stringByAppendingPathComponent:[NSString stringWithFormat:@"%@%@",ASMA_BASEDIR,dbASMA_entries[indexPath.row].fullpath]];
			success = [fileManager fileExistsAtPath:checkPath];
			if (success) dbASMA_entries[indexPath.row].downloaded=1;
			else dbASMA_entries[indexPath.row].downloaded=0;
		}
		if (dbASMA_entries[indexPath.row].downloaded==0) {
            topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0f];
        }
		bottomLabel.text = dbASMA_entries[indexPath.row].fullpath;
	}
    
	return cell;
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {

}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value>=1) [self doSecAction:indexPath];
	else [self doPrimAction:indexPath];
}

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    }
}


@end
