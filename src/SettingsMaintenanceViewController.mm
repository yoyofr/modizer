//
//  SettingsMaintenanceViewController.m
//  modizer
//
//  Created by Yohann Magnien on 10/08/13.
//
//

#import "SettingsMaintenanceViewController.h"
#import "ImagesCache.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;

#import "TTFadeAnimator.h"

@interface SettingsMaintenanceViewController ()
@end

@implementation SettingsMaintenanceViewController

@synthesize tableView,detailViewController,rootVC;

#include "MiniPlayerImplementTableView.h"
#include "AlertsCommonFunctions.h"

-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:YES];
    else {
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"")];
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////


- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}


- (void)viewDidLoad
{
    [super viewDidLoad];
    
    
    
    self.navigationController.delegate = self;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithCustomView: btn];
    self.navigationItem.rightBarButtonItem = item;
    
    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
    
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
    
    /////////////////////////////////////
    // Waiting view
    /////////////////////////////////////
    waitingView = [[WaitingView alloc] init];
    waitingView.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingView];
    
    NSDictionary *views = NSDictionaryOfVariableBindings(waitingView);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingView(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingView(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    

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

- (void)viewWillAppear:(BOOL)animated {
    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
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
    
    [self hideWaiting];
    
    [super viewWillAppear:animated];
}

-(void) viewDidAppear:(BOOL)animated {
//    [self.tableView reloadData];
    [super viewDidAppear:animated];
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void) resetSettings {
    [self showWaiting];
    [self flushMainLoop];
    
    //remove settings from userpref
    NSString *appDomain = [[NSBundle mainBundle] bundleIdentifier];
    [[NSUserDefaults standardUserDefaults] removePersistentDomainForName:appDomain];
    //
    //load default
    [SettingsGenViewController applyDefaultSettings];
    [detailViewController settingsChanged:(int)SETTINGS_ALL];
    
    [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Settings reseted",@"")];
    [self hideWaiting];
    
}

-(bool) resetRatingsDB {
    [self showWaiting];
    [self flushMainLoop];
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[256];
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		snprintf(sqlStatement,sizeof(sqlStatement),"UPDATE user_stats SET rating=NULL");
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
	};
	sqlite3_close(db);

    [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Ratings reseted",@"")];
    [self hideWaiting];
    
	return TRUE;
}

-(bool) resetPlaycountDB {
    [self showWaiting];
    [self flushMainLoop];
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[256];
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        snprintf(sqlStatement,sizeof(sqlStatement),"UPDATE user_stats SET play_count=0");
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
	};
	sqlite3_close(db);
    
    [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Played Counters reseted",@"")];
    [self hideWaiting];
	return TRUE;
}

-(bool) clearPNqueue {
    if (detailViewController) {
        [detailViewController stop];
        [detailViewController clearQueue];
        [self hideMiniPlayer];
        //[detailViewController refresh]
    }
}

-(bool) cleanDB {
    [self showWaiting];
    [self flushMainLoop];
	
    DBHelper::cleanDB();
	
    [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Database cleaned",@"")];
    [self hideWaiting];
	return TRUE;
}

-(void) recreateSamplesFolder {
    [self showWaiting];
    [self flushMainLoop];
    
    [rootVC createSamplesFromPackage:TRUE];
    
    [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Samples folder created",@"")];
    [self hideWaiting];
}

-(void) clearImageCache {
    ImagesCache *imagesCache = [[ImagesCache alloc] init];
    [imagesCache cleanCache];
    
    [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Cache cleaned",@"")];
}

-(void) resetDB {
    [self showWaiting];
    [self flushMainLoop];
    [rootVC createEditableCopyOfDatabaseIfNeeded:TRUE quiet:TRUE];
    [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Database reseted",@"")];
    [self hideWaiting];
}

-(void) removeCurrentCover {
    [self showWaiting];
    [self flushMainLoop];
    NSError *err;
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    NSString *currentPlayFilepath =[detailViewController getCurrentModuleFilepath];
    if (currentPlayFilepath==nil) {
        
        [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"No cover to remove",@"")];
        [self hideWaiting];
        return;
    }
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.jpg",NSHomeDirectory(),[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.png",NSHomeDirectory(),[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.gif",NSHomeDirectory(),[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.jpg",NSHomeDirectory(),[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.png",NSHomeDirectory(),[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.gif",NSHomeDirectory(),[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
    mFileMngr=nil;
    
    [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Cover removed",@"")];
    [self hideWaiting];
}


#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return 9;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    NSString *title=nil;
    return title;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    NSString *footer=nil;
    return footer;
}

- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    NSString *cellValue;
    const NSInteger TOP_LABEL_TAG = 1001;
    UILabel *topLabel;
    BButton *btn;
    
    UITableViewCell *cell = [tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    if ((cell == nil)||forceReloadCells) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
        cell.frame=CGRectMake(0,0,self.view.frame.size.width,50);
        cell.autoresizingMask=UIViewAutoresizingFlexibleWidth;
        [cell setBackgroundColor:[UIColor clearColor]];
        
        NSString *imgName=(darkMode?@"tabview_gradient50Black.png":@"tabview_gradient50.png");
        UIImage *image = [UIImage imageNamed:imgName];
        UIImageView *imageView = [[UIImageView alloc] initWithImage:image];
        imageView.contentMode = UIViewContentModeScaleToFill;
        cell.backgroundView = imageView;
        //[imageView release];
        
        
        btn= [[BButton alloc] initWithFrame:CGRectMake(self.view.frame.size.width/2-100,
                                                      10,
                                                      200,          
                                                       30)];
        btn.tag=TOP_LABEL_TAG;
        [cell.contentView addSubview:btn];
        btn.autoresizingMask=UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleRightMargin;
        
        cell.accessoryView=nil;
        cell.accessoryType = UITableViewCellAccessoryNone;
        [cell layoutSubviews];
    } else {
        btn = (BButton *)[cell viewWithTag:TOP_LABEL_TAG];
    }    
    btn.frame=CGRectMake((self.view.frame.size.width-self.view.frame.size.width*2/3)/2,10,self.view.frame.size.width*2/3,30);
    
    NSString *txt;
    switch (indexPath.row) {            
        case 0: //Clean DB
            txt=NSLocalizedString(@"Clean Database",@"");
            [btn setType:BButtonTypePrimary];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(cleanDB) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 1: //Clean listening now
            txt=NSLocalizedString(@"Clear 'Now Playing' queue",@"");
            [btn setType:BButtonTypePrimary];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(clearPNqueue) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 2: //Recreate Samples folder
            txt=NSLocalizedString(@"Recreate Samples folder",@"");
            [btn setType:BButtonTypePrimary];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(recreateSamplesFolder) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 3: //Reset settings to default
            txt=NSLocalizedString(@"Reset settings to default",@"");
            [btn setType:BButtonTypeWarning];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetSettings) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 4: //Remove current cover
            txt=NSLocalizedString(@"Remove current cover",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(removeCurrentCover) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 5: //Reset Ratings
            txt=NSLocalizedString(@"Reset Ratings",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetRatingsDB) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 6: //Reset played counter
            txt=NSLocalizedString(@"Reset Played Counters",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetPlaycountDB) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 7: //Reset DB
            txt=NSLocalizedString(@"Reset Database",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetDB) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 8: //Clear image cache
            txt=NSLocalizedString(@"Clear images cache",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(clearImageCache) forControlEvents:UIControlEventTouchUpInside];
            
            break;
            

    }
    [btn setTitle:txt forState:UIControlStateNormal];
    
    
    return cell;
}

/*
 // Override to support conditional editing of the table view.
 - (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath
 {
 // Return NO if you do not want the specified item to be editable.
 return YES;
 }
 */

/*
 // Override to support editing the table view.
 - (void)tableView:(UITableView *)tabView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
 {
 if (editingStyle == UITableViewCellEditingStyleDelete) {
 // Delete the row from the data source
 [tabView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
 }
 else if (editingStyle == UITableViewCellEditingStyleInsert) {
 // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
 }
 }
 */

/*
 // Override to support rearranging the table view.
 - (void)tableView:(UITableView *)tabView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
 {
 }
 */

/*
 // Override to support conditional rearranging of the table view.
 - (BOOL)tableView:(UITableView *)tabView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
 {
 // Return NO if you do not want the item to be re-orderable.
 return YES;
 }
 */

- (void)dealloc
{
    [waitingView removeFromSuperview];
    waitingView=nil;
}

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    }
}


#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tabView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
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
