//
//  RootViewControllerWebParser.mm
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#define GET_NB_ENTRIES 1

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
#import "RootViewControllerWebParser.h"
#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
#import "WebBrowser.h"
#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];
#import "QuartzCore/CAAnimation.h"

#import "TTFadeAnimator.h"

@implementation RootViewControllerWebParser

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

#include "MiniPlayerImplementTableView.h"

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
    NSString *machine = [[NSString alloc] initWithFormat:@"%s",name];
    
    // Done with this
    free(name);
    
    return machine;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////


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
                if (search_db) {
                    if (search_db_hasFiles) download_all=1;
                } else {
                    if (db_hasFiles) download_all=1;
                }
                csection=indexPath.section-1-download_all;
                
                if (csection>=0) {
                    //display popup
                    t_dbHVSC_browse_entry **cur_db_entries;
                    cur_db_entries=(search_db?search_db_entries:db_entries);
                    
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
    
    self.navigationController.delegate = self;
            
    forceReloadCells=false;
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    mFileMngr=[[NSFileManager alloc] init];
    
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
    
    db_nb_entries=0;
    search_db_nb_entries=0;
    
    search_db_hasFiles=0;
    db_hasFiles=0;
    
    mSearchText=nil;
    mCurrentWinAskedDownload=0;
    mClickedPrimAction=0;
    list=nil;
    keys=nil;
    
    if (browse_depth==0) {
        mNbFileEntries=DBHelper::getNbFilesEntries();
    }
    
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithCustomView: btn];
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
    
    
    if (shouldFillKeys) {
        shouldFillKeys=0;
        [self fillKeysWithDB_Dir];
    } else { //reset downloaded, rating & playcount flags
        for (int i=0;i<db_nb_entries;i++) {
            db_entries_data[i].downloaded=-1;
            db_entries_data[i].rating=-1;
            db_entries_data[i].playcount=-1;
        }
    }
}

-(void) fillKeysWithDB_Dir {
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
        search_db_entries_data=(t_dbHVSC_browse_entry*)calloc(1,db_nb_entries*sizeof(t_dbHVSC_browse_entry));
        
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
                    search_db_entries[i][search_db_entries_count[i]].filesize=db_entries[i][j].filesize;
                    
                    search_db_entries[i][search_db_entries_count[i]].id_md5=db_entries[i][j].id_md5;
                    search_db_entries[i][search_db_entries_count[i]].fullpath=db_entries[i][j].fullpath;
                    search_db_entries[i][search_db_entries_count[i]].dir1=db_entries[i][j].dir1;
                    search_db_entries[i][search_db_entries_count[i]].dir2=db_entries[i][j].dir2;
                    search_db_entries[i][search_db_entries_count[i]].dir3=db_entries[i][j].dir3;
                    
                    search_db_entries_count[i]++;
                    search_db_nb_entries++;
                }
            }
        }
        return;
    }
    
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++) {
            db_entries_data[i].label=nil;
            db_entries_data[i].fullpath=nil;
            db_entries_data[i].id_md5=nil;
            db_entries_data[i].dir1=nil;
            db_entries_data[i].dir2=nil;
            db_entries_data[i].dir3=nil;
        }
        free(db_entries_data);db_entries_data=NULL;
        db_nb_entries=0;
    }
    db_nb_entries=10;
    if (db_nb_entries) {
        //2nd initialize array to receive entries
        db_entries_data=(t_dbWEB_browse_entry *)calloc(1,db_nb_entries*sizeof(t_dbWEB_browse_entry));
        memset(db_entries_data,0,db_nb_entries*sizeof(t_db_browse_entry));
        db_entries_index=0;
        for (int i=0;i<27;i++) {
            db_entries_count[i]=0;
            db_entries[i]=NULL;
        }
        
        index=-1;
        
        char *str="tototestXX";
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
        db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",str];
        db_entries[index][db_entries_count[index]].filesize=0;
        
        db_entries_count[index]++;
        db_entries_index++;
    }
}

-(void) traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    bool oldmode=darkMode;
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    [self.tableView reloadData];
}

-(void) viewWillAppear:(BOOL)animated {
    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    [self.sBar setBarStyle:UIBarStyleDefault];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    
    self.navigationController.delegate = self;
    
    bool oldmode=darkMode;
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    
    if ([detailViewController mPlaylist_size]>0) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    } else {
        wasMiniPlayerOn=false;
    }
    
    if (keys) {
        //[keys release];
        keys=nil;
    }
    if (list) {
        //[list release];
        list=nil;
    }
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
    
    if (detailViewController.mShouldHaveFocus) {
        detailViewController.mShouldHaveFocus=0;
        [self.navigationController pushViewController:detailViewController animated:YES];
    } else {
        if (shouldFillKeys) {
            
            [self showWaiting];
            [self flushMainLoop];
            
            [self fillKeys];
            [tableView reloadData];
            [self hideWaiting];
        }
    }
    [super viewWillAppear:animated];
    
}
-(void) refreshViewAfterDownload {
    if (childController) [(RootViewControllerWebParser*)childController refreshViewAfterDownload];
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
    
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}

- (void)viewDidDisappear:(BOOL)animated {
    [self hideWaiting];
    /*if (childController) {
        [childController viewDidDisappear:FALSE];
    }*/
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
    if (section==0) return 0;
    //Check if "Get all entries" has to be displayed
    if (search_db) {
        if (search_db_hasFiles) {
            if (section==1) return @"";
            if (search_db_entries_count[section-2]) return [indexTitlesDownload objectAtIndex:section];
            return nil;
        }
        if (search_db_entries_count[section-1]) return [indexTitles objectAtIndex:section];
        return nil;
    } else {
        if (db_hasFiles) {
            if (section==1) return @"";
            if (db_entries_count[section-2]) return [indexTitlesDownload objectAtIndex:section];
            return nil;
        }
        if (db_entries_count[section-1]) return [indexTitles objectAtIndex:section];
        return nil;
    }
    return nil;
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    local_flag=0;
    
    //Check if "Get all entries" has to be displayed
    if (search_db) {
        if (search_db_hasFiles) return 28+1;
        return 28;
    } else {
        if (db_hasFiles) return 28+1;
        return 28;
    }
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section==0) return 0;
    //Check if "Get all entries" has to be displayed
    if (search_db) {
        if (search_db_hasFiles) {
            if (section==1) return 1;
            return search_db_entries_count[section-2];
        }
        return search_db_entries_count[section-1];
    } else {
        if (db_hasFiles) {
            if (section==1) return 1;
            return db_entries_count[section-2];
        }
        return db_entries_count[section-1];
    }
}
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    if (mSearch) return nil;
    if (search_db) {
        if (search_db_hasFiles) return indexTitlesDownload;
    } else {
        if (db_hasFiles) return indexTitlesDownload;
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
    if ((cell == nil)||forceReloadCells) {
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
        topLabel.font = [UIFont boldSystemFontOfSize:18];
        topLabel.lineBreakMode=NSLineBreakByTruncatingMiddle;
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
        bottomLabel.lineBreakMode=NSLineBreakByTruncatingMiddle;
        bottomLabel.opaque=TRUE;
        
        bottomImageView = [[UIImageView alloc] initWithImage:nil];
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
    
    if (darkMode) {
        topLabel.textColor = [UIColor colorWithRed:1-0.1 green:1-0.1 blue:1-0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:1-0.9 green:1-0.9 blue:1-0.9 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:1-0.4 green:1-0.4 blue:1-0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:1-0.8 green:1-0.8 blue:1-0.8 alpha:1.0];
    } else {
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
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
    t_dbWEB_browse_entry **cur_db_entries;
    cur_db_entries=(search_db?search_db_entries:db_entries);
    int section = indexPath.section-1;
    int download_all=0;
    if (search_db) {
        if (search_db_hasFiles) download_all=1;
    } else {
        if (db_hasFiles) download_all=1;
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
                pathToCheck=[NSString stringWithFormat:@"%@/Documents/%@%@",NSHomeDirectory(),downloadbaseDir,cur_db_entries[section][indexPath.row].fullpath];
            if (pathToCheck) {
                if ([mFileMngr fileExistsAtPath:pathToCheck]) cur_db_entries[section][indexPath.row].downloaded=1;
                else cur_db_entries[section][indexPath.row].downloaded=0;
            } else cur_db_entries[section][indexPath.row].downloaded=1;
        }
        
        if(cur_db_entries[section][indexPath.row].downloaded==1) {
            colFactor=1;
        } else colFactor=0;
        
        if (cur_db_entries[section][indexPath.row].isFile) { //FILE
            if (colFactor==0) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0];
            topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                       22);
            if (cur_db_entries[section][indexPath.row].downloaded==1) {
                
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
            if (darkMode) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:1.0f alpha:1.0f];
            else topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
            
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
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
        cur_db_entries=(search_dbASMA?search_dbASMA_entries:dbASMA_entries);
        int section = indexPath.section-1;
        int download_all=0;
        if (search_dbASMA) {
            if (search_dbASMA_hasFiles) download_all=1;
        } else {
            if (dbASMA_hasFiles) download_all=1;
        }
        section-=download_all;
        //delete file
        NSString *fullpath=[NSHomeDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents/%@%@",ASMA_BASEDIR,cur_db_entries[section][indexPath.row].fullpath]];
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
    t_dbHVSC_browse_entry **cur_db_entries=(search_dbASMA?search_dbASMA_entries:dbASMA_entries);
    int section =indexPath.section-1;
    if (search_dbASMA) {
        if (search_dbASMA_hasFiles) section--;
    } else {
        if (dbASMA_hasFiles) section--;
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
    shouldFillKeys=1;
    search_dbASMA=0;
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
    search_dbASMA=0;
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
        UIAlertView *nofileplaying=[[UIAlertView alloc] initWithTitle:@"Warning"
                                                               message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
        [nofileplaying show];
    }
}

#pragma mark -
#pragma mark Table view delegate
- (void) primaryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    
    if (browse_depth==0) {
        
    } else {
        t_dbHVSC_browse_entry **cur_db_entries;
        cur_db_entries=(search_dbASMA?search_dbASMA_entries:dbASMA_entries);
        int section = indexPath.section-1;
        int download_all=0;
        if (search_dbASMA) {
            if (search_dbASMA_hasFiles) download_all=1;
        } else {
            if (dbASMA_hasFiles) download_all=1;
        }
        section-=download_all;
        
        if (cur_db_entries[section][indexPath.row].id_md5) { //FILE
            //File selected, start download is needed
            NSString *sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[section][indexPath.row].label];
            NSString *ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[section][indexPath.row].fullpath];
            NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",ASMA_BASEDIR,cur_db_entries[section][indexPath.row].fullpath];
            
            if (cur_db_entries[section][indexPath.row].downloaded==1) {
                NSMutableArray *array_label = [[NSMutableArray alloc] init];
                NSMutableArray *array_path = [[NSMutableArray alloc] init];
                [array_label addObject:sidFilename];
                [array_path addObject:localPath];
                cur_db_entries[section][indexPath.row].rating=-1;
                [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                
                if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                else [tableView reloadData];
            } else {
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                mCurrentWinAskedDownload=1;
                
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
    
    [self hideWaiting];
    
    
}
- (void) secondaryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    
    if (browse_depth==0) {
    } else {
        t_dbHVSC_browse_entry **cur_db_entries;
        cur_db_entries=(search_dbASMA?search_dbASMA_entries:dbASMA_entries);
        int download_all=0;
        int section=indexPath.section-1;
        if (search_dbASMA) {
            if (search_dbASMA_hasFiles) download_all=1;
        } else {
            if (dbASMA_hasFiles) download_all=1;
        }
        section-=download_all;
        
        if (cur_db_entries[section][indexPath.row].id_md5) { //FILE
            //File selected, start download is needed
            NSString *sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[section][indexPath.row].label];
            NSString *ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[section][indexPath.row].fullpath];
            NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",ASMA_BASEDIR,cur_db_entries[section][indexPath.row].fullpath];
            mClickedPrimAction=2;
            
            if (cur_db_entries[section][indexPath.row].downloaded==1) {
                NSMutableArray *array_label = [[NSMutableArray alloc] init];
                NSMutableArray *array_path = [[NSMutableArray alloc] init];
                [array_label addObject:sidFilename];
                [array_path addObject:localPath];
                [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                
                cur_db_entries[section][indexPath.row].rating=-1;
                if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                else [tableView reloadData];
            } else {
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                mCurrentWinAskedDownload=1;
                
                NSString *asma_url=[NSString stringWithFormat:@"%s",settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text];
                NSRange nsr=[asma_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                if (nsr.location==NSNotFound) {
                    //HTTP
                    [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",asma_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                } else {
                    //FTP
                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[asma_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                }
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
        cur_db_entries=(search_dbASMA?search_dbASMA_entries:dbASMA_entries);
        int section = indexPath.section-1;
        int download_all=0;
        if (search_dbASMA) {
            if (search_dbASMA_hasFiles) download_all=1;
        } else {
            if (dbASMA_hasFiles) download_all=1;
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
            
            int *cur_db_entries_count=(search_dbASMA?search_dbASMA_entries_count:dbASMA_entries_count);
            
            for (int i=0;i<27;i++) {
                for (int j=0;j<cur_db_entries_count[i];j++) {
                    if (cur_db_entries[i][j].id_md5) {//mod found
                        
                        existing=cur_db_entries[i][j].downloaded;
                        if (existing==-1) {
                            NSString *pathToCheck=nil;
                            
                            if (cur_db_entries[i][j].fullpath)
                                pathToCheck=[NSString stringWithFormat:@"%@/Documents/%@%@",NSHomeDirectory(),ASMA_BASEDIR,cur_db_entries[i][j].fullpath];
                            if (pathToCheck) {
                                if ([mFileMngr fileExistsAtPath:pathToCheck]) cur_db_entries[i][j].downloaded=1;
                                else existing=cur_db_entries[i][j].downloaded=0;
                            } else existing=cur_db_entries[i][j].downloaded=1;
                        }
                        if (existing==0) {
                            
                            //File selected, start download is needed
                            sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[i][j].label];
                            ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[i][j].fullpath];
                            localPath=[NSString stringWithFormat:@"Documents/%@%@",ASMA_BASEDIR,cur_db_entries[i][j].fullpath];
                            mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
                            
                            [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                            mCurrentWinAskedDownload=1;
                            
                            NSString *asma_url=[NSString stringWithFormat:@"%s",settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text];
                            NSRange nsr=[asma_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                            
                            if (first) {
                                if (nsr.location==NSNotFound) {
                                    //HTTP
                                    if ([downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",asma_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:1]
                                        ) {
                                        tooMuch=1;
                                        break;
                                    }
                                } else {
                                    //FTP
                                    if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[asma_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:1]) {
                                        tooMuch=1;
                                        break;
                                    }
                                }
                                first=0;
                            } else {
                                
                                if (nsr.location==NSNotFound) {
                                    //HTTP
                                    if ([downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",asma_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:2] ) {
                                        tooMuch=1;
                                        break;
                                    }
                                } else {
                                    //FTP
                                    if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[asma_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:2]) {
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
                NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",ASMA_BASEDIR,cur_db_entries[section][indexPath.row].fullpath];
                mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
                
                if (cur_db_entries[section][indexPath.row].downloaded==1) {
                    if (mClickedPrimAction) {
                        NSMutableArray *array_label = [[NSMutableArray alloc] init];
                        NSMutableArray *array_path = [[NSMutableArray alloc] init];
                        [array_label addObject:sidFilename];
                        [array_path addObject:localPath];
                        cur_db_entries[section][indexPath.row].rating=-1;
                        [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                        if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                        
                        if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                        else [tabView reloadData];
                    } else {
                        if ([detailViewController add_to_playlist:localPath fileName:sidFilename forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)]) {
                            if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                            
                            cur_db_entries[section][indexPath.row].rating=-1;
                            if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                            else [tabView reloadData];
                        }
                    }
                } else {
                    [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                    mCurrentWinAskedDownload=1;
                    
                    NSString *asma_url=[NSString stringWithFormat:@"%s",settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text];
                    NSRange nsr=[asma_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                    if (nsr.location==NSNotFound) {
                        //HTTP
                        [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",asma_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                    } else {
                        //FTP
                        [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[asma_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                    }
                    
                }
            } else { //DIR
                if (browse_depth==1) {//DIR1
                    mDir1=cur_db_entries[section][indexPath.row].dir1;
                } else if (browse_depth==2) {//DIR2
                    mDir2=cur_db_entries[section][indexPath.row].dir2;
                } else if (browse_depth==3) {//DIR3
                    mDir3=cur_db_entries[section][indexPath.row].dir3;
                }
                
                if (childController == nil) childController = [[RootViewControllerWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                else {// Don't cache childviews
                }
                
                childController.title = cur_db_entries[section][indexPath.row].label;
                // Set new depth
                ((RootViewControllerWebParser*)childController)->browse_depth = browse_depth+1;
                ((RootViewControllerWebParser*)childController)->detailViewController=detailViewController;
                ((RootViewControllerWebParser*)childController)->downloadViewController=downloadViewController;
                
                ((RootViewControllerWebParser*)childController)->mDir1 = mDir1;
                ((RootViewControllerWebParser*)childController)->mDir2 = mDir2;
                ((RootViewControllerWebParser*)childController)->mDir3 = mDir3;
                
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
    //[waitingView release];
    
    //[currentPath release];
    if (mSearchText) {
        //[mSearchText release];
        mSearchText=nil;
    }
    if (keys) {
        //[keys release];
        keys=nil;
    }
    if (list) {
        //[list release];
        list=nil;
    }	
    
    if (dbASMA_nb_entries) {
        for (int i=0;i<dbASMA_nb_entries;i++) {
            dbASMA_entries_data[i].label=nil;
            dbASMA_entries_data[i].fullpath=nil;
            dbASMA_entries_data[i].id_md5=nil;
            dbASMA_entries_data[i].dir1=nil;
            dbASMA_entries_data[i].dir2=nil;
            dbASMA_entries_data[i].dir3=nil;
            
        }
        free(dbASMA_entries_data);
    }
    if (search_dbASMA_nb_entries) {
        free(search_dbASMA_entries_data);
    }
    
    if (indexTitles) {
        //[indexTitles release];
        indexTitles=nil;
    }
    if (indexTitlesDownload) {
        //[indexTitlesDownload release];
        indexTitlesDownload=nil;
    }
    
    
    if (mFileMngr) {
        //[mFileMngr release];
        mFileMngr=nil;
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


@end
