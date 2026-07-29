//
//  SettingsMaintenanceViewController.mm
//  modizer
//
//  Created by Yohann Magnien on 10/08/13.
//
//


#import "SettingsMaintenanceViewController.h"
#import "ImagesCache.h"
#import "ModizFileHelper.h"
#import "DownloadViewController.h"
#import "DBHelper.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;

extern volatile t_settings settings[MAX_SETTINGS];

extern bool dbhelper_cancel;

#import "TTFadeAnimator.h"

@interface SettingsMaintenanceViewController ()
@end

@implementation SettingsMaintenanceViewController

@synthesize tableView,detailViewController,rootVC,downloadViewController;

#include "MiniPlayerImplementTableView.h"
#include "AlertsCommonFunctions.h"
#include "PlaylistCommonFunctions.h"

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


- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];

    [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        // Force reload cells during transition
        forceReloadCells = true;
        [self.tableView reloadData];
    } completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        // Ensure final layout is correct
        [self.tableView reloadData];
    }];
}

- (void)viewDidLoad
{
    START_PROFILE
    [super viewDidLoad];
    
    if ([NSProcessInfo processInfo].isiOSAppOnMac) {
        self.hidesBottomBarWhenPushed = YES;
    } else if (@available(iOS 18.0, *)) {
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
            self.hidesBottomBarWhenPushed = YES;
        }
    }
    
    self.navigationController.delegate = self;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:NOW_PLAYING_ICON] style:UIBarButtonItemStylePlain target:self action:@selector(goPlayer)];
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
    
    views = NSDictionaryOfVariableBindings(waitingViewPlayer);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    END_PROFILE
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
//    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
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
 
    
    [self hideWaiting];
    
    [super viewWillAppear:animated];
}

- (void)viewDidDisappear:(BOOL)animated {
    [repeatingTimer invalidate];
    repeatingTimer = nil;
    
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView removeObserver:self
                                          forKeyPath:observedSelector
                                             context:LoadingProgressObserverContext];
    [super viewDidDisappear:animated];
    
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
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[256];
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		snprintf(sqlStatement,sizeof(sqlStatement),"UPDATE user_stats SET rating=NULL");
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else MDZELog("ErrSQL : %d",err);
	};
	sqlite3_close(db);

    [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Ratings reseted",@"")];
    [self hideWaiting];
    
	return TRUE;
}

-(bool) resetPlaycountDB {
    [self showWaiting];
    [self flushMainLoop];
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[256];
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,sizeof(sqlStatement),"UPDATE user_stats SET play_count=0");
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else MDZELog("ErrSQL : %d",err);
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

extern char cleanDB_Status[256];
-(void) cleanDBUpdStatus {
    [self updateWaitingDetail:[NSString stringWithFormat:@"%s",cleanDB_Status]];
}

-(bool) cleanDB {
    //[self showWaiting];
    //[self flushMainLoop];
    dbhelper_cancel=false;
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Warning",@"")
                                                                   message:NSLocalizedString(@"Are you sure you want to clean the DB, it might take a while ?",@"")
                                                            preferredStyle:UIAlertControllerStyleAlert];
    
    UIAlertAction* cleanAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Clean",@"") style:UIAlertActionStyleDestructive
                                                         handler:^(UIAlertAction * action) {
        
        [self showWaitingCancel];
        [self hideWaitingProgress];
        [self updateWaitingTitle:NSLocalizedString(@"Cleaning DB",@"")];
        [self updateWaitingDetail:@""];
        
        [self showWaiting];
        
        repeatingTimerCleanDB = [NSTimer scheduledTimerWithTimeInterval: 0.20f target:self selector:@selector(cleanDBUpdStatus) userInfo:nil repeats: YES]; //5 times/second
        
        dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
            //Background Thread
            dbhelper_cancel=false;
            DBHelper::cleanDB();
            
            dispatch_async(dispatch_get_main_queue(), ^(void){
                //Run UI Updates
                [self hideWaiting];
                
                [repeatingTimerCleanDB invalidate];
                repeatingTimerCleanDB = nil;
            });
        });
    }];
    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
                                                         handler:^(UIAlertAction * action) {
    }];
    
    [alert addAction:cancelAction];
    [alert addAction:cleanAction];
    [self presentViewController:alert animated:YES completion:nil];
	
    //[self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Database cleaned",@"")];
    //[self hideWaiting];
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
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.jpg",[ModizFileHelper getAppHomeDirectory],[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.png",[ModizFileHelper getAppHomeDirectory],[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.gif",[ModizFileHelper getAppHomeDirectory],[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.jpg",[ModizFileHelper getAppHomeDirectory],[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.png",[ModizFileHelper getAppHomeDirectory],[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.gif",[ModizFileHelper getAppHomeDirectory],[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
    mFileMngr=nil;
    
    [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Cover removed",@"")];
    [self hideWaiting];
}


#pragma mark - Download of missing playlists entries

//
// Online collections local folders, as created by the OnlineViewController collections browsers.
// Only the collections for which the remote location can be rebuilt from the local path are
// handled: the WEB parsed ones (AMP, JoshW, VGMRips, SNESmusic, SMS Power!, ZXArt) build their
// download URL while parsing the web pages, and that URL is not stored anywhere locally.
//
#define MDZ_AMP_BASEDIR @"AMP"

//collections whose remote location can be rebuilt from the local path, shown as-is to the user
#define MDZ_RESOLVABLE_COLLECTIONS @"MODLAND, HVSC, ASMA, CGSC"

-(bool) isOnlineCollectionBaseDir:(NSString*)baseDir {
    return ([baseDir isEqualToString:MODLAND_BASEDIR]||
            [baseDir isEqualToString:HVSC_BASEDIR]||
            [baseDir isEqualToString:ASMA_BASEDIR]||
            [baseDir isEqualToString:CGSC_BASEDIR]||
            [baseDir isEqualToString:MDZ_AMP_BASEDIR]||
            [baseDir isEqualToString:JOSHW_BASEDIR]||
            [baseDir isEqualToString:VGMR_BASEDIR]||
            [baseDir isEqualToString:SNESmusic_BASEDIR]||
            [baseDir isEqualToString:SMSP_BASEDIR]||
            [baseDir isEqualToString:ZXART_BASEDIR]);
}

//recursive lookup: the DownloadViewController can be a tab child, sit in a navigation stack,
//or be hidden in the "More" navigation controller of the tab bar
- (id)findChildOfClass:(Class)cls inViewController:(UIViewController*)vc {
    if (vc==nil) return nil;
    if ([vc isKindOfClass:cls]) return vc;

    if ([vc isKindOfClass:[UITabBarController class]]) {
        for (UIViewController *child in ((UITabBarController*)vc).viewControllers) {
            id res=[self findChildOfClass:cls inViewController:child];
            if (res) return res;
        }
        id res=[self findChildOfClass:cls inViewController:((UITabBarController*)vc).moreNavigationController];
        if (res) return res;
        return nil;
    }
    if ([vc isKindOfClass:[UINavigationController class]]) {
        for (UIViewController *child in ((UINavigationController*)vc).viewControllers) {
            id res=[self findChildOfClass:cls inViewController:child];
            if (res) return res;
        }
        return nil;
    }
    for (UIViewController *child in vc.childViewControllers) {
        id res=[self findChildOfClass:cls inViewController:child];
        if (res) return res;
    }
    return nil;
}

-(void) loadDownloadController {
    if (downloadViewController) return;

    //1/ walk up from ourselves: we are pushed in the same tab bar hierarchy
    for (UIViewController *vc=self; vc!=nil; vc=vc.parentViewController) {
        if ([vc isKindOfClass:[UITabBarController class]]) {
            downloadViewController=[self findChildOfClass:[DownloadViewController class] inViewController:vc];
            if (downloadViewController) return;
        }
    }

    //2/ fallback: scan every window of every connected scene
    NSMutableArray *windows=[NSMutableArray array];
    for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]]) [windows addObjectsFromArray:((UIWindowScene*)scene).windows];
    }
    if ([windows count]==0) {
        if ([UIApplication sharedApplication].keyWindow) [windows addObject:[UIApplication sharedApplication].keyWindow];
    }
    for (UIWindow *window in windows) {
        downloadViewController=[self findChildOfClass:[DownloadViewController class] inViewController:window.rootViewController];
        if (downloadViewController) return;
    }
}

-(void) checkCreateForLocalPath:(NSString*)localPath {
    NSString *completePath=[[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:localPath] stringByDeletingLastPathComponent];
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    [mFileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:nil];
}

//build a download request, as used by the collections browsers (cf RootViewController<collection>.mm)
-(NSDictionary*) buildURLRequest:(NSString*)url localPath:(NSString*)localPath fileName:(NSString*)fileName isMODLAND:(int)isMODLAND {
    return @{@"type":@"url",@"url":url,@"local":localPath,@"name":fileName,@"ismodland":@(isMODLAND)};
}

-(NSDictionary*) buildFTPRequest:(NSString*)remotePath host:(NSString*)host localPath:(NSString*)localPath fileName:(NSString*)fileName isMODLAND:(int)isMODLAND {
    return @{@"type":@"ftp",@"remote":remotePath,@"host":host,@"local":localPath,@"name":fileName,@"ismodland":@(isMODLAND)};
}

//
// Rebuild the remote location of a missing playlist entry.
// Returns the list of downloads to queue (main file + additional required ones), or nil if the
// entry does not belong to a collection which can be resolved offline.
//
-(NSArray*) buildDownloadRequestsForLocalPath:(NSString*)localPath baseDir:(NSString*)baseDir {
    NSString *fileName=[localPath lastPathComponent];
    //path relative to the collection root, starting with a '/'
    NSString *remotePath=[localPath substringFromIndex:[[NSString stringWithFormat:@"Documents/%@",baseDir] length]];
    NSString *collection_url=nil;

    if ([baseDir isEqualToString:MODLAND_BASEDIR]) {
        //MODLAND local layout is author/filetype[/album]/filename, remote one is filetype/author[/album]/filename
        NSString *fullpath=DBHelper::getFullPathFromLocalPath([remotePath substringFromIndex:1]);
        if (fullpath==nil) return nil;  //not in the MODLAND DB anymore
        NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",fullpath];
        collection_url=[NSString stringWithFormat:@"%s",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text];
        if ([collection_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch].location==NSNotFound) {
            return @[[self buildURLRequest:[NSString stringWithFormat:@"%@%@",collection_url,ftpPath] localPath:localPath fileName:fileName isMODLAND:1]];
        }
        return @[[self buildFTPRequest:ftpPath host:[collection_url substringFromIndex:6] localPath:localPath fileName:fileName isMODLAND:1]];
    }

    if ([baseDir isEqualToString:HVSC_BASEDIR]||[baseDir isEqualToString:ASMA_BASEDIR]) {
        if ([baseDir isEqualToString:HVSC_BASEDIR]) collection_url=[NSString stringWithFormat:@"%s",settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text];
        else collection_url=[NSString stringWithFormat:@"%s",settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text];
        if ([collection_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch].location==NSNotFound) {
            return @[[self buildURLRequest:[NSString stringWithFormat:@"%@%@",collection_url,remotePath] localPath:localPath fileName:fileName isMODLAND:1]];
        }
        return @[[self buildFTPRequest:remotePath host:[collection_url substringFromIndex:6] localPath:localPath fileName:fileName isMODLAND:1]];
    }

    if ([baseDir isEqualToString:CGSC_BASEDIR]) {
        //CGSC is HTTP only, and comes with optional companion files
        collection_url=[NSString stringWithFormat:@"%s",settings[ONLINE_CGSC_CURRENT_URL].detail.mdz_msgbox.text];
        NSMutableArray *requests=[NSMutableArray array];
        NSArray *addExt=@[@"str",@"wds",@"pic",@"pgg",@"pjj"];
        for (NSString *ext in addExt) {
            NSString *addName=[[fileName stringByDeletingPathExtension] stringByAppendingFormat:@".%@",ext];
            NSString *addLocal=[[localPath stringByDeletingPathExtension] stringByAppendingFormat:@".%@",ext];
            NSString *addRemote=[[remotePath stringByDeletingPathExtension] stringByAppendingFormat:@".%@",ext];
            [requests addObject:[self buildURLRequest:[NSString stringWithFormat:@"%@%@",collection_url,addRemote] localPath:addLocal fileName:addName isMODLAND:3]];
        }
        [requests addObject:[self buildURLRequest:[NSString stringWithFormat:@"%@%@",collection_url,remotePath] localPath:localPath fileName:fileName isMODLAND:2]];
        return requests;
    }

    return nil;
}

-(void) queueDownloadRequests:(NSArray*)requests {
    for (NSDictionary *req in requests) {
        NSString *localPath=[req objectForKey:@"local"];
        [self checkCreateForLocalPath:localPath];
        if ([[req objectForKey:@"type"] isEqualToString:@"ftp"]) {
            [downloadViewController addFTPToDownloadList:localPath
                                                  ftpURL:[req objectForKey:@"remote"]
                                                 ftpHost:[req objectForKey:@"host"]
                                                filesize:-1
                                                filename:[req objectForKey:@"name"]
                                               isMODLAND:[[req objectForKey:@"ismodland"] intValue]
                                        usePrimaryAction:0];
        } else {
            [downloadViewController addURLToDownloadList:[req objectForKey:@"url"]
                                                fileName:[req objectForKey:@"name"]
                                                filePath:localPath
                                                filesize:-1
                                               isMODLAND:[[req objectForKey:@"ismodland"] intValue]
                                        usePrimaryAction:0];
        }
    }
}

//gather every entry of every user playlist (label + fullpath)
-(NSArray*) collectPlaylistsEntries {
    NSMutableArray *entries=[NSMutableArray array];
    t_playlist_DB *plList=NULL;
    int plListSize=[self loadPlayListsListFromDB:&plList];

    for (int i=0;i<plListSize;i++) {
        NSMutableArray *labels=[NSMutableArray array];
        NSMutableArray *fullpaths=[NSMutableArray array];
        [self loadUserList:plList[i].pl_id labels:labels fullpaths:fullpaths];
        for (int j=0;j<[fullpaths count];j++) {
            [entries addObject:@[[labels objectAtIndex:j],[fullpaths objectAtIndex:j]]];
        }
        mdz_safe_free(plList[i].pl_name);
    }
    mdz_safe_free(plList);

    return entries;
}

-(void) scanPlaylistsAndDownloadMissingEntries {
    [self hideWaitingCancel];
    [self hideWaitingProgress];
    [self updateWaitingTitle:NSLocalizedString(@"Checking playlists",@"")];
    [self updateWaitingDetail:@""];
    [self showWaiting];

    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
        //Background Thread
        NSArray *entries=[self collectPlaylistsEntries];
        NSFileManager *mFileMngr=[[NSFileManager alloc] init];
        NSMutableArray *requests=[NSMutableArray array];
        NSMutableSet *alreadyChecked=[NSMutableSet set];
        NSMutableSet *unresolvedCollections=[NSMutableSet set];
        int nb_entries=(int)[entries count];
        int nb_queued=0,nb_present=0,nb_notonline=0,nb_unresolved=0;

        for (int i=0;i<nb_entries;i++) {
            if ((i%16)==0) {
                dispatch_async(dispatch_get_main_queue(), ^(void){
                    [self updateWaitingDetail:[NSString stringWithFormat:@"%d/%d",i,nb_entries]];
                });
            }

            //remove the archive index (@) and subsong index (?) suffixes if any
            NSString *localPath=[ModizFileHelper getFullCleanFilePath:[[entries objectAtIndex:i] objectAtIndex:1]];
            if (localPath==nil) continue;
            if ([alreadyChecked containsObject:localPath]) continue;
            [alreadyChecked addObject:localPath];

            NSArray *comp=[localPath componentsSeparatedByString:@"/"];
            if (([comp count]<3)||(![[comp objectAtIndex:0] isEqualToString:@"Documents"])) {
                nb_notonline++;
                continue;
            }
            NSString *baseDir=[comp objectAtIndex:1];
            if (![self isOnlineCollectionBaseDir:baseDir]) {
                nb_notonline++;
                continue;
            }

            if ([mFileMngr fileExistsAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:localPath]]) {
                nb_present++;
                continue;
            }

            NSArray *entryRequests=[self buildDownloadRequestsForLocalPath:localPath baseDir:baseDir];
            if (entryRequests==nil) {
                //WEB parsed collection, or entry not found in the collection DB anymore
                nb_unresolved++;
                [unresolvedCollections addObject:baseDir];
                continue;
            }
            [requests addObjectsFromArray:entryRequests];
            nb_queued++;
        }

        dispatch_async(dispatch_get_main_queue(), ^(void){
            //Run UI Updates
            [self queueDownloadRequests:requests];
            [self hideWaiting];

            NSMutableArray *msgLines=[NSMutableArray array];
            [msgLines addObject:[NSString stringWithFormat:NSLocalizedString(@"%d file(s) queued for download.",@""),nb_queued]];
            [msgLines addObject:[NSString stringWithFormat:NSLocalizedString(@"%d file(s) already available.",@""),nb_present]];
            [msgLines addObject:[NSString stringWithFormat:NSLocalizedString(@"%d file(s) not from an online collection.",@""),nb_notonline]];
            if (nb_unresolved) {
                NSArray *sortedCollections=[[unresolvedCollections allObjects] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
                [msgLines addObject:[NSString stringWithFormat:NSLocalizedString(@"%d file(s) cannot be resolved (%@), browse the collection to download them.",@""),
                                     nb_unresolved,[sortedCollections componentsJoinedByString:@", "]]];
            }
            [self showAlertMsg:NSLocalizedString(@"Info",@"") message:[msgLines componentsJoinedByString:@"\n"]];
        });
    });
}

-(void) downloadMissingPlaylistsEntries {
    [self loadDownloadController];
    if (downloadViewController==nil) {
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Download manager is not available.",@"")];
        return;
    }

    NSString *message=[NSString stringWithFormat:@"%@\n\n%@\n%@",
                       NSLocalizedString(@"Check all playlists entries and queue the download of the missing files coming from online collections ?",@""),
                       NSLocalizedString(@"Supported collections:",@""),
                       MDZ_RESOLVABLE_COLLECTIONS];

    UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Info",@"")
                                                                  message:message
                                                           preferredStyle:UIAlertControllerStyleAlert];

    UIAlertAction* checkAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Check",@"") style:UIAlertActionStyleDefault
                                                       handler:^(UIAlertAction * action) {
        [self scanPlaylistsAndDownloadMissingEntries];
    }];
    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
                                                         handler:^(UIAlertAction * action) {
    }];

    [alert addAction:cancelAction];
    [alert addAction:checkAction];
    [self presentViewController:alert animated:YES completion:nil];
}


#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return 10;
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
    
    if (forceReloadCells) {
        while ([tabView dequeueReusableCellWithIdentifier:CellIdentifier]) {}
        forceReloadCells=false;
    }
    
    UITableViewCell *cell = [tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];

        cell.frame=CGRectMake(0,0,tabView.bounds.size.width,50);
        cell.autoresizingMask=UIViewAutoresizingFlexibleWidth;
//        [cell setBackgroundColor:[UIColor clearColor]];
//        UIBackgroundConfiguration *backgroundConfig = [UIBackgroundConfiguration listGroupedCellConfiguration];
//        backgroundConfig.backgroundColor = [UIColor systemGroupedBackgroundColor];
//        cell.backgroundConfiguration = backgroundConfig;
        UIBackgroundConfiguration *backgroundConfig = [UIBackgroundConfiguration listGroupedCellConfiguration];
        backgroundConfig.backgroundColor = [UIColor systemGroupedBackgroundColor];
        cell.backgroundConfiguration = backgroundConfig;
        
        // Pour gérer automatiquement l'état sélectionné, utilise configurationUpdateHandler
        cell.configurationUpdateHandler = ^(UITableViewCell *cell, UICellConfigurationState *state) {
            UIBackgroundConfiguration *config = [cell backgroundConfiguration];
            if (state.selected || state.highlighted) {
                config.backgroundColor = [UIColor secondarySystemGroupedBackgroundColor];
                //config.backgroundColor = [UIColor systemBlueColor];
                config.cornerRadius = 8.0;
            } else {
                config.backgroundColor = [UIColor systemGroupedBackgroundColor];
                config.cornerRadius = 0.0;
            }
            [cell setBackgroundConfiguration:config];
        };
        

        btn= [[BButton alloc] initWithFrame:CGRectMake(tabView.bounds.size.width/2-100,
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
    btn.frame=CGRectMake((tabView.bounds.size.width-tabView.bounds.size.width*2/3)/2,10,tabView.bounds.size.width*2/3,30);
    
    float margin=MDZ_TABVIEW_SEPARATOR_MARGIN;
    cell.layoutMargins = UIEdgeInsetsMake(0, margin, 0, margin);
    cell.separatorInset = UIEdgeInsetsMake(0, margin, 0, margin);
    
    NSString *txt;
    switch (indexPath.row) {
        case 0: //Download missing playlists entries
            txt=NSLocalizedString(@"Download missing playlists files",@"");
            [btn setType:BButtonTypeSuccess];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(downloadMissingPlaylistsEntries) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 1: //Clean DB
            txt=NSLocalizedString(@"Clean Database",@"");
            [btn setType:BButtonTypePrimary];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(cleanDB) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 2: //Clean listening now
            txt=NSLocalizedString(@"Clear 'Now Playing' queue",@"");
            [btn setType:BButtonTypePrimary];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(clearPNqueue) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 3: //Recreate Samples folder
            txt=NSLocalizedString(@"Recreate Samples folder",@"");
            [btn setType:BButtonTypePrimary];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(recreateSamplesFolder) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 4: //Reset settings to default
            txt=NSLocalizedString(@"Reset settings to default",@"");
            [btn setType:BButtonTypeWarning];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetSettings) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 5: //Remove current cover
            txt=NSLocalizedString(@"Remove current cover",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(removeCurrentCover) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 6: //Reset Ratings
            txt=NSLocalizedString(@"Reset Ratings",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetRatingsDB) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 7: //Reset played counter
            txt=NSLocalizedString(@"Reset Played Counters",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetPlaycountDB) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 8: //Reset DB
            txt=NSLocalizedString(@"Reset Database",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetDB) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 9: //Clear image cache
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
    [waitingViewPlayer removeFromSuperview];
    waitingView=nil;
    waitingViewPlayer=nil;
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
    
    dbhelper_cancel=true;
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
