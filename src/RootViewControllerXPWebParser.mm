//
//  RootViewControllerXPWebParser.mm
//  modizer1
//
//  Created by Yohann Magnien on 10/03/24.
//  Copyright __YoyoFR / Yohann Magnien__ 2024. All rights reserved.
//


#import "RootViewControllerXPWebParser.h"


@implementation RootViewControllerXPWebParser

@synthesize mFileMngr;
@synthesize detailViewController;
@synthesize downloadViewController;
@synthesize tableView,sBar;
@synthesize childController;
@synthesize mSearchText;
@synthesize popTipView;
@synthesize mWebBaseURL,rootDir;

#pragma mark -
#pragma mark Alert functions

#import "AlertsCommonFunctions.h"

#pragma mark -
#pragma mark Miniplayer
#include "MiniPlayerImplementTableView.h"

#pragma mark -
#pragma mark Search functions
#import "SearchCommonFunctions.h"

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    }
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
    {
        if (indexPath != nil) {
            if ((gestureRecognizer.state==UIGestureRecognizerStateBegan)||(gestureRecognizer.state==UIGestureRecognizerStateChanged)) {
                //                    NSLog(@"long press on table view at %d/%d", indexPath.section,indexPath.row);
                int crow=indexPath.row;
                int csection;
                
                
                csection=indexPath.section-1;
                
                if (csection>=0) {
                    //display popup
                    t_WEB_browse_entry **cur_db_entries;
                    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
                    
                    NSString *str=cur_db_entries[csection][crow].fullpath;
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
    
    dictActionBtn=[NSMutableDictionary dictionaryWithCapacity:64];
    
    indexTitleMode=0;
    
    mPopupAnimation=0;
    
    sort_mode=0;
    
    self.navigationController.delegate = self;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    mFileMngr=[[NSFileManager alloc] init];
    
    imagesCache = [[ImagesCache alloc] init];
    
    navbarTitle=[[UILabel alloc] init];
    navbarTitle.userInteractionEnabled=TRUE;
    
    if (browse_depth>0) {
        self.navigationItem.titleView=navbarTitle;
        
        navbarTitle.text=self.title;
        self.navigationItem.title=navbarTitle.text;
        
    }
    
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
    //    self.tableView.backgroundColor = [UIColor blackColor];
    
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
    
    search_dbWEB=0;  //reset to ensure search_dbWEB is not used by default
    
    dbWEB_nb_entries=0;
    search_dbWEB_nb_entries=0;
    
    search_dbWEB_hasFiles=0;
    dbWEB_hasFiles=0;
    
    mSearchText=nil;
    mClickedPrimAction=0;
    
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithCustomView: btn];
    self.navigationItem.rightBarButtonItem = item;
    
    indexTitles = [[NSMutableArray alloc] init];
    [indexTitles addObject:@"{search}"];
    if (indexTitleMode) {
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
    }
    
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
    
    [super viewDidLoad];
    
    end_time=clock();
#ifdef LOAD_PROFILE
    NSLog(@"rootview : %d",end_time-start_time);
#endif
}

-(void) fillKeys {
    //to be implemented in each parser
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
    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
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
    
    if (childController) {
        //[childController release];
        childController = NULL;
    }
    
    //Reset rating if applicable (ensure updated value)
    if (dbWEB_nb_entries) {
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].rating=-1;
        }
    }
    if (search_dbWEB_nb_entries) {
        for (int i=0;i<search_dbWEB_nb_entries;i++) {
            search_dbWEB_entries_data[i].rating=-1;
        }
    }
    /////////////
    
    [super viewWillAppear:animated];
    
}
-(void) refreshViewAfterDownload {
    if (childController) [(RootViewControllerXPWebParser*)childController refreshViewAfterDownload];
    else {
        [self fillKeys];
        [tableView reloadData];
    }
}

- (void)checkCreate:(NSString *)filePath {
    //NSString *completePath=[NSString stringWithFormat:@"%@/%@",NSHomeDirectory(),filePath];
    NSError *err;
    [mFileMngr createDirectoryAtPath:filePath withIntermediateDirectories:TRUE attributes:nil error:&err];
}

- (void)viewDidAppear:(BOOL)animated {
    [self hideWaiting];
    
    [super viewDidAppear:animated];
    
    if (shouldFillKeys) {
        
        [self updateWaitingTitle:NSLocalizedString(@"Loading & parsing",@"")];
        [self updateWaitingDetail:@""];
        [self showWaiting];
        [self flushMainLoop];
        
        [self fillKeys];
        [tableView reloadData];
        
        [self hideWaiting];
    } else {
        [self fillKeys];
        [tableView reloadData];
    }
    
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}

- (void)viewDidDisappear:(BOOL)animated {
    [self hideWaiting];
    /*if (childController) {
     [childController viewDidDisappear:FALSE];
     }*/
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

- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    if (mSearch) return nil;
    
    return indexTitles;
}


- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (!indexTitleMode) return nil;
    if (mSearch) return nil;
    if (section==0) return 0;
    if (search_dbWEB) {
        if (search_dbWEB_entries_count[section-1]) return [indexTitles objectAtIndex:section];
        return nil;
    } else {
        if (dbWEB_entries_count[section-1]) return [indexTitles objectAtIndex:section];
        return nil;
    }
    return nil;
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    if (search_dbWEB) {
        if (indexTitleMode) return 28;
        return 2;
    } else {
        if (indexTitleMode) return 28;
        return 2;
    }
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section==0) return 0;
    //Check if "Get all entries" has to be displayed
    if (search_dbWEB) {
        return search_dbWEB_entries_count[section-1];
    } else {
        return dbWEB_entries_count[section-1];
    }
}

- (NSInteger)tableView:(UITableView *)tabView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    if (mSearch) return -1;
    if (index == 0) {
        [tabView setContentOffset:CGPointZero animated:NO];
        return NSNotFound;
    }
    return index;
}

// Override to support editing the table view.
- (void)tableView:(UITableView *)tabView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        //delete entry
        t_WEB_browse_entry **cur_db_entries;
        cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
        int section = indexPath.section-1;
        
        //delete file
        NSString *fullpath=[NSHomeDirectory() stringByAppendingFormat:@"/%@",cur_db_entries[section][indexPath.row].fullpath];
        NSError *err;
        //            NSLog(@"%@",fullpath);
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
    t_WEB_browse_entry **cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    int section =indexPath.section-1;
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
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    
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
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    if (mSearch) shouldFillKeys=1;
    search_dbWEB=0;
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
    //shouldFillKeys=1;
    search_dbWEB=0;
    [self fillKeys];
    
    [tableView reloadData];
}
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar {
    [searchBar resignFirstResponder];
}

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
    
    {
        t_WEB_browse_entry **cur_db_entries;
        cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
        int section = indexPath.section-1;
        
        if (cur_db_entries[section][indexPath.row].isFile) { //FILE
            //File selected, start download is needed
            NSString *localPath=[NSHomeDirectory() stringByAppendingFormat:@"/%@",cur_db_entries[section][indexPath.row].fullpath];
            
            if (cur_db_entries[section][indexPath.row].downloaded==1) {
                NSMutableArray *array_label = [[NSMutableArray alloc] init];
                NSMutableArray *array_path = [[NSMutableArray alloc] init];
                [array_label addObject:cur_db_entries[section][indexPath.row].label];
                [array_path addObject:cur_db_entries[section][indexPath.row].fullpath];
                cur_db_entries[section][indexPath.row].rating=-1;
                [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                
                [tableView reloadData];
            } else {
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                
                [downloadViewController addURLToDownloadList:cur_db_entries[section][indexPath.row].URL fileName:cur_db_entries[section][indexPath.row].label filePath:cur_db_entries[section][indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:1];
                
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
    
    t_WEB_browse_entry **cur_db_entries;
    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    int section=indexPath.section-1;
    
    if (cur_db_entries[section][indexPath.row].isFile) { //FILE
        //File selected, start download is needed
        NSString *localPath=[NSHomeDirectory() stringByAppendingFormat:@"/%@",cur_db_entries[section][indexPath.row].fullpath];
        mClickedPrimAction=2;
        
        if (cur_db_entries[section][indexPath.row].downloaded==1) {
            //add to playlist
            [self addToPlaylistSelView:cur_db_entries[section][indexPath.row].fullpath label:cur_db_entries[section][indexPath.row].label showNowListening:true];
            
            cur_db_entries[section][indexPath.row].rating=-1;
            [tableView reloadData];
        } else {
            [self checkCreate:[localPath stringByDeletingLastPathComponent]];
            
            [downloadViewController addURLToDownloadList:cur_db_entries[section][indexPath.row].URL fileName:cur_db_entries[section][indexPath.row].label filePath:cur_db_entries[section][indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
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

- (void)tableView:(UITableView *)tabView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    t_WEB_browse_entry **cur_db_entries;
    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    int section=indexPath.section-1;
    
    if (cur_db_entries[section][indexPath.row].isFile) { //FILE
        //File selected, start download is needed
        NSString *localPath=[NSHomeDirectory() stringByAppendingFormat:@"/%@",cur_db_entries[section][indexPath.row].fullpath];
        mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
        
        if (cur_db_entries[section][indexPath.row].downloaded==1) {
            if (mClickedPrimAction) {
                NSMutableArray *array_label = [[NSMutableArray alloc] init];
                NSMutableArray *array_path = [[NSMutableArray alloc] init];
                [array_label addObject:cur_db_entries[section][indexPath.row].label];
                [array_path addObject:cur_db_entries[section][indexPath.row].fullpath];
                cur_db_entries[section][indexPath.row].rating=-1;
                [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                
                [tabView reloadData];
            } else {
                if ([detailViewController add_to_playlist:localPath fileName:cur_db_entries[section][indexPath.row].label forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)]) {
                    if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                    
                    cur_db_entries[section][indexPath.row].rating=-1;
                    [tabView reloadData];
                }
            }
        } else {
            [self checkCreate:[localPath stringByDeletingLastPathComponent]];
            
            
            [downloadViewController addURLToDownloadList:cur_db_entries[section][indexPath.row].URL fileName:cur_db_entries[section][indexPath.row].label filePath:cur_db_entries[section][indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
            
        }
    } else {
        childController = [[RootViewControllerXPWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
        //set new title
        childController.title = cur_db_entries[section][indexPath.row].fullpath;
        // Set new directory
        ((RootViewControllerXPWebParser*)childController)->browse_depth = browse_depth+1;
        ((RootViewControllerXPWebParser*)childController)->detailViewController=detailViewController;
        ((RootViewControllerXPWebParser*)childController)->downloadViewController=downloadViewController;
        ((RootViewControllerXPWebParser*)childController)->mWebBaseURL=cur_db_entries[section][indexPath.row].URL;
        
        childController.view.frame=self.view.frame;
        // And push the window
        [self.navigationController pushViewController:childController animated:YES];
    }
}


#pragma mark -
#pragma mark popup functions

-(void) hidePopup {
    infoMsgView.hidden=YES;
    mPopupAnimation=0;
}

-(void) openPopup:(NSString *)msg {
    CGRect frame;
    if (mPopupAnimation) return;
    mPopupAnimation=1;
    infoMsgView.layer.zPosition=0xFFFF;
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
    if (mSearchText) {
        mSearchText=nil;
    }
    if (dbWEB_nb_entries) {
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].label=nil;
            dbWEB_entries_data[i].fullpath=nil;
            dbWEB_entries_data[i].URL=nil;
            dbWEB_entries_data[i].info=nil;
            dbWEB_entries_data[i].img_URL=nil;
        }
        free(dbWEB_entries_data);
    }
    if (search_dbWEB_nb_entries) {
        for (int i=0;i<27;i++) {
            for (int j=0;j<search_dbWEB_entries_count[i];j++) {
                search_dbWEB_entries[i][j].label=nil;
                search_dbWEB_entries[i][j].fullpath=nil;
                search_dbWEB_entries[i][j].URL=nil;
                search_dbWEB_entries[i][j].info=nil;
                search_dbWEB_entries[i][j].img_URL=nil;
            }
            search_dbWEB_entries[i]=NULL;
        }
        search_dbWEB_nb_entries=0;
        free(search_dbWEB_entries_data);
    }
    if (indexTitles) {
        indexTitles=nil;
    }
    if (mFileMngr) {
        mFileMngr=nil;
    }
    //[super dealloc];
}

#pragma mark -
#pragma mark - UINavigationControllerDelegate

- (id <UIViewControllerAnimatedTransitioning>)navigationController:(UINavigationController *)navigationController
                                   animationControllerForOperation:(UINavigationControllerOperation)operation
                                                fromViewController:(UIViewController *)fromVC
                                                  toViewController:(UIViewController *)toVC
{
    return [[TTFadeAnimator alloc] init];
}


@end
