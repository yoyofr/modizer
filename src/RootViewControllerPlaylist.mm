//
//  RootViewController.mm
//  modizer
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#define RATING_IMG(a) ( (a==5?2:(a?1:0)) )

#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define ROW_HEIGHT 40



#define LIMITED_LIST_SIZE 1024

#import "MDZUIImageView.h"
#import "ModizFileHelper.h"

#include <sys/types.h>
#include <sys/sysctl.h>

#include "gme.h"

//SPC parser
#include "SPCTagParser.h"


#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidTuneInfo.h"

#include "unzip.h"

#include <pthread.h>

extern pthread_mutex_t db_mutex;
static pthread_mutex_t db_mutexHVSCSTIL;
static 	int	newPlaylist;
//static int shouldFillKeys;
static int local_flag;
static volatile int mPopupAnimation=0;

extern "C" {
#include "common/md5.h"
}

static char browser_song_md5[33];
static char *browser_stil_info;//[MAX_STIL_DATA_LENGTH];
static char **browser_sidtune_title,**browser_sidtune_name;


#import "AppDelegate_Phone.h"
#import "RootViewControllerPlaylist.h"
#import "DetailViewControllerIphone.h"
#import "WebBrowser.h"
#import "QuartzCore/CAAnimation.h"
#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];

#import "TTFadeAnimator.h"


@implementation RootViewControllerPlaylist

@synthesize mFileMngr,mDetailPlayerMode;
@synthesize detailViewController;
@synthesize tableView,sBar;
@synthesize list;
@synthesize keys;
@synthesize currentPath;
@synthesize childController;
@synthesize mSearchText;
@synthesize popTipView;
@synthesize currentPlayedEntry;
@synthesize repeatTimer,activeKey;

#pragma mark -
#pragma mark Search functions
#include "SearchCommonFunctions.h"

#pragma mark -
#pragma mark Miniplayer functions
#include "MiniPlayerImplementTableView.h"

#pragma mark -
#pragma mark Alerts popup functions
#include "AlertsCommonFunctions.h"

#pragma mark -
#pragma mark Playlist functions
#include "PlaylistCommonFunctions.h"

#pragma mark -
#pragma mark Waiting view functions
#include "WaitingViewCommonMethods.h"

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
    [super setEditing:editing animated:animated];
    [tableView setEditing:editing animated:animated];
    if (editing==FALSE) {
        if (mDetailPlayerMode) self.navigationItem.rightBarButtonItem = nil;
        else {
            UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:NOW_PLAYING_ICON] style:UIBarButtonItemStylePlain target:self action:@selector(goPlayer)];
            self.navigationItem.rightBarButtonItem = item;
        }
    }
    [tableView reloadData];
}

int qsort_ComparePlaylistEntries(const void *entryA, const void *entryB) {
    NSString *strA,*strB;
    NSComparisonResult res;
    strA=((t_playlist_entry*)entryA)->label;
    strB=((t_playlist_entry*)entryB)->label;
    res=[strA localizedCaseInsensitiveCompare:strB];
    if (res==NSOrderedAscending) return -1;
    if (res==NSOrderedSame) return 0;
    return 1; //NSOrderedDescending
}

int qsort_ComparePlaylistEntriesRev(const void *entryA, const void *entryB) {
    NSString *strA,*strB;
    NSComparisonResult res;
    strA=((t_playlist_entry*)entryA)->label;
    strB=((t_playlist_entry*)entryB)->label;
    res=[strB localizedCaseInsensitiveCompare:strA];
    if (res==NSOrderedAscending) return -1;
    if (res==NSOrderedSame) return 0;
    return 1; //NSOrderedDescending
}

int qsort_ComparePlaylistEntriesFP(const void *entryA, const void *entryB) {
    NSString *strA,*strB;
    NSComparisonResult res;
    strA=((t_playlist_entry*)entryA)->fullpath;
    strB=((t_playlist_entry*)entryB)->fullpath;
    res=[strA localizedCaseInsensitiveCompare:strB];
    if (res==NSOrderedAscending) return -1;
    if (res==NSOrderedSame) return 0;
    return 1; //NSOrderedDescending
}

int qsort_ComparePlaylistEntriesRevFP(const void *entryA, const void *entryB) {
    NSString *strA,*strB;
    NSComparisonResult res;
    strA=((t_playlist_entry*)entryA)->fullpath;
    strB=((t_playlist_entry*)entryB)->fullpath;
    res=[strB localizedCaseInsensitiveCompare:strA];
    if (res==NSOrderedAscending) return -1;
    if (res==NSOrderedSame) return 0;
    return 1; //NSOrderedDescending
}


- (void)createNewPlaylist {
    UIAlertController *alertC = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Enter playlist name",@"")
                                                                    message:nil
                                                             preferredStyle:UIAlertControllerStyleAlert];
    __weak UIAlertController *weakAlert = alertC;
    [alertC addTextFieldWithConfigurationHandler:^(UITextField *textField) {
        textField.placeholder = @"";
    }];
    
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
        [self freePlaylist];
        
    }];
    [alertC addAction:cancelAction];
    
    UIAlertAction *saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Create",@"") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        UITextField *plName = weakAlert.textFields.firstObject;
        if (![plName.text isEqualToString:@""]) {
            if (childController == nil) childController = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            
            ((RootViewControllerPlaylist*)childController)->show_playlist=1;
            
            playlist->playlist_id=nil;
            playlist->playlist_name=nil;
            playlist->playlist_name=[[NSString alloc] initWithString:plName.text];
            playlist->playlist_id=[self minitNewPlaylistDB:playlist->playlist_name];
            self.navigationItem.title=playlist->playlist_name;
            
            //set new title
            childController.title = playlist->playlist_name;
            
            ((RootViewControllerPlaylist*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerPlaylist*)childController)->detailViewController=detailViewController;
            ((RootViewControllerPlaylist*)childController)->playlist=playlist;
            ((RootViewControllerPlaylist*)childController)->mFreePlaylist=0;
//            childController.view.frame=self.view.frame;
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
            keys=nil;
            list=nil;
            
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
        }
        else{
            [self presentViewController:weakAlert animated:YES completion:nil];
        }
    }];
    [alertC addAction:saveAction];
    
    [self showAlert:alertC];
}

- (void)saveToNewPlaylist {
    UIAlertController *alertC = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Enter playlist name",@"")
                                                                    message:nil
                                                             preferredStyle:UIAlertControllerStyleAlert];
    __weak UIAlertController *weakAlert = alertC;
    [alertC addTextFieldWithConfigurationHandler:^(UITextField *textField) {
        textField.placeholder = @"";
    }];
    
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
    }];
    [alertC addAction:cancelAction];
    
    UIAlertAction *saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Save",@"") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        UITextField *plName = weakAlert.textFields.firstObject;
        if (![plName.text isEqualToString:@""]) {
            playlist->playlist_id=nil;
            playlist->playlist_name=nil;
            playlist->playlist_name=[[NSString alloc] initWithString:plName.text];
            playlist->playlist_id=[self minitNewPlaylistDB:playlist->playlist_name];
            integrated_playlist=0;
            [self addListToPlaylistDB];
            self.navigationItem.title=playlist->playlist_name;
            [self.tableView reloadData];
        }
        else{
            [self presentViewController:weakAlert animated:YES completion:nil];
        }
    }];
    [alertC addAction:saveAction];
    
    [self showAlert:alertC];
}

-(void) deletePlaylist {
    UIAlertController *alertC = [UIAlertController alertControllerWithTitle:NSLocalizedString(NSLocalizedString(@"Warning",@""),@"")
                                                                    message:NSLocalizedString(@"Are you sure you want to delete this playlist ?",@"")
                                                             preferredStyle:UIAlertControllerStyleAlert];
    
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
    }];
    [alertC addAction:cancelAction];
    
    UIAlertAction *deleteAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Delete",@"") style:UIAlertActionStyleDestructive handler:^(UIAlertAction * _Nonnull action) {
        [self deletePlaylistDB:playlist->playlist_id];
        [self.navigationController popViewControllerAnimated:YES];
    }];
    [alertC addAction:deleteAction];
    
    [self showAlert:alertC];
}

- (void)renamePlaylist {
    UIAlertController *alertC = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Enter new name",@"")
                                                                    message:nil
                                                             preferredStyle:UIAlertControllerStyleAlert];
    __weak UIAlertController *weakAlert = alertC;
    [alertC addTextFieldWithConfigurationHandler:^(UITextField *textField) {
        textField.text = playlist->playlist_name;
    }];
    
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
    }];
    [alertC addAction:cancelAction];
    
    UIAlertAction *saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Rename",@"") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        UITextField *plName = weakAlert.textFields.firstObject;
        if (![plName.text isEqualToString:@""]) {
            playlist->playlist_name=nil;
            playlist->playlist_name=[[NSString alloc] initWithString:plName.text];
            [self updatePlaylistNameDB:playlist->playlist_id playlist_name:playlist->playlist_name];
            self.navigationItem.title=[NSString stringWithFormat:@"%@",playlist->playlist_name];
        }
        else{
            [self presentViewController:weakAlert animated:YES completion:nil];
        }
    }];
    [alertC addAction:saveAction];
    
    [self showAlert:alertC];
}

-(void) exportFiles {
    UIAlertController *alertC = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Export playlist related files to a dedicated folder\nPlease enter a folder name:",@"")
                                                                    message:nil
                                                             preferredStyle:UIAlertControllerStyleAlert];
    __weak UIAlertController *weakAlert = alertC;
    [alertC addTextFieldWithConfigurationHandler:^(UITextField *textField) {
        textField.text = playlist->playlist_name;
    }];
    
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
    }];
    [alertC addAction:cancelAction];
    
    UIAlertAction *exportAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Export",@"") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        UITextField *plName = weakAlert.textFields.firstObject;
        if (![plName.text isEqualToString:@""]) {
            
            NSFileManager *fileMgr=[NSFileManager defaultManager];
            NSString *tgtFolder=[NSString stringWithFormat:@"%@/Documents/Exports/%@",[ModizFileHelper getAppHomeDirectory],plName.text];
            NSError *error;
            if (![fileMgr createDirectoryAtPath:tgtFolder
                                           withIntermediateDirectories:YES
                                                            attributes:nil
                                                                 error:&error]) {
                [self showAlertMsg:@"Error" message:NSLocalizedString(@"Cannot create export folder",@"")];
                MDZELog("Create directory error: %@", error);
            } else {
                NSString *srcPath,*tgtPath;
                for (int i=0;i<playlist->nb_entries;i++) {
                    srcPath=[ModizFileHelper getFullPathForFilePath:playlist->entries[i].fullpath];
                    tgtPath=[NSString stringWithFormat:@"%@/%@",tgtFolder,[playlist->entries[i].fullpath lastPathComponent]];
                    if (![fileMgr copyItemAtPath:srcPath toPath:tgtPath error:&error]) {
                        MDZELog("Error during copy of %@ to %@, error %@",srcPath,tgtPath,error);
                    }
                }
                [self showAlertMsg:@"Information" message:NSLocalizedString(@"Export done",@"")];
            }
        }
        else {
            [self presentViewController:weakAlert animated:YES completion:nil];
        }
    }];
    [alertC addAction:exportAction];
    
    [self showAlert:alertC];
}

- (void)shufflePlaylist {
    if (playlist->nb_entries) {
        int pos=0;
        
        [tableView reloadData];
        
        //Shuffle playlist
        srand(time(NULL));
        
        int rowofs=(integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING?1:2);
        for (int from_idx=0;from_idx<playlist->nb_entries;from_idx++) {
            int to_idx=rand()%(playlist->nb_entries);
            if (from_idx!=to_idx) {
                if (show_playlist) {
                    signed char tmpR=playlist->entries[from_idx].ratings;
                    short int tmpC=playlist->entries[from_idx].playcounts;
                    NSString *tmpF=playlist->entries[from_idx].fullpath;
                    NSString *tmpL=playlist->entries[from_idx].label;
                    
                    playlist->entries[from_idx].label=playlist->entries[to_idx].label;
                    playlist->entries[from_idx].fullpath=playlist->entries[to_idx].fullpath;
                    playlist->entries[from_idx].ratings=playlist->entries[to_idx].ratings;
                    playlist->entries[from_idx].playcounts=playlist->entries[to_idx].playcounts;
                    
                    playlist->entries[to_idx].label=tmpL;
                    playlist->entries[to_idx].fullpath=tmpF;
                    playlist->entries[to_idx].ratings=tmpR;
                    playlist->entries[to_idx].playcounts=tmpC;
                    
                    if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) {
                        t_plPlaylist_entry tmpF;
                        tmpF=detailViewController.mPlaylist[from_idx];
                        detailViewController.mPlaylist[from_idx]=detailViewController.mPlaylist[to_idx];
                        detailViewController.mPlaylist[to_idx]=tmpF;
                        
                        /*if ((from_idx>detailViewController.mPlaylist_pos)&&(to_idx<=detailViewController.mPlaylist_pos)) detailViewController.mPlaylist_pos++;
                         else if ((from_idx<detailViewController.mPlaylist_pos)&&(to_idx>=detailViewController.mPlaylist_pos)) detailViewController.mPlaylist_pos--;
                         else if (from_idx==detailViewController.mPlaylist_pos) detailViewController.mPlaylist_pos=to_idx;*/
                        if (detailViewController.mPlaylist_pos==from_idx) detailViewController.mPlaylist_pos=to_idx;
                        else if (detailViewController.mPlaylist_pos==to_idx) detailViewController.mPlaylist_pos=from_idx;
                        
                        detailViewController.mShouldUpdateInfos=1;
                    }
                }
            }
        }
        
        [tableView reloadData];
        
        if (playlist->playlist_id) [self replacePlaylistDBwithCurrent];
        
        if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) {
            currentPlayedEntry=detailViewController.mPlaylist_pos;
            NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
            int pos=currentPlayedEntry+1;
            if ((mDetailPlayerMode==0) && (integrated_playlist==0)) pos++;
            if (pos<[self.tableView numberOfRowsInSection:0]) [self.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:pos] animated:NO scrollPosition:UITableViewScrollPositionMiddle];
        }
        [self updateMiniPlayer];
    }
}
- (void)sortAZPlaylist:(bool)fullpath {
    if (playlist->nb_entries) {
        
        NSString *tmpFP;
        if (!(playlist->playlist_id)) {
            tmpFP=[NSString stringWithString:detailViewController.mPlaylist[detailViewController.mPlaylist_pos].mPlaylistFilepath];
        }
        
        if (fullpath) qsort(playlist->entries,playlist->nb_entries,sizeof(t_playlist_entry),qsort_ComparePlaylistEntriesFP);
        else qsort(playlist->entries,playlist->nb_entries,sizeof(t_playlist_entry),qsort_ComparePlaylistEntries);
        
        
        if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) {
            for (int i=0;i<detailViewController.mPlaylist_size;i++) {
                detailViewController.mPlaylist[i].mPlaylistFilename=[[NSString alloc] initWithString:playlist->entries[i].label];
                detailViewController.mPlaylist[i].mPlaylistFilepath=[[NSString alloc] initWithString:playlist->entries[i].fullpath];
                
                detailViewController.mPlaylist[i].mPlaylistRating=playlist->entries[i].ratings;
                detailViewController.mPlaylist[i].mPlaylistCount=0;
                
                detailViewController.mPlaylist[i].cover_flag=-1;
                if ([detailViewController.mPlaylist[i].mPlaylistFilepath isEqualToString:tmpFP]) {
                    detailViewController.mPlaylist_pos=i;
                }
            }
            
            detailViewController.mShouldUpdateInfos=1;
        }
        
        if (playlist->playlist_id) [self replacePlaylistDBwithCurrent];
        [self.tableView reloadData];
        
        
        if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) {
            currentPlayedEntry=detailViewController.mPlaylist_pos;
            NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
            int pos=currentPlayedEntry+1;
            if ((mDetailPlayerMode==0) && (integrated_playlist==0)) pos++;
            if (pos<[self.tableView numberOfRowsInSection:0]) [self.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:pos] animated:NO scrollPosition:UITableViewScrollPositionMiddle];
        }
        
        
        [self updateMiniPlayer];
    }
}
- (void)sortZAPlaylist:(bool)fullpath {
    if (playlist->nb_entries) {
        NSString *tmpFP;
        if (!(playlist->playlist_id)) {
            tmpFP=[NSString stringWithString:detailViewController.mPlaylist[detailViewController.mPlaylist_pos].mPlaylistFilepath];
        }
        
        if (fullpath) qsort(playlist->entries,playlist->nb_entries,sizeof(t_playlist_entry),qsort_ComparePlaylistEntriesRevFP);
        else qsort(playlist->entries,playlist->nb_entries,sizeof(t_playlist_entry),qsort_ComparePlaylistEntriesRev);
        
        
        if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) {
            for (int i=0;i<detailViewController.mPlaylist_size;i++) {
                detailViewController.mPlaylist[i].mPlaylistFilename=[[NSString alloc] initWithString:playlist->entries[i].label];
                detailViewController.mPlaylist[i].mPlaylistFilepath=[[NSString alloc] initWithString:playlist->entries[i].fullpath];
                
                detailViewController.mPlaylist[i].mPlaylistRating=playlist->entries[i].ratings;
                detailViewController.mPlaylist[i].mPlaylistCount=0;
                
                detailViewController.mPlaylist[i].cover_flag=-1;
                if ([detailViewController.mPlaylist[i].mPlaylistFilepath isEqualToString:tmpFP]) {
                    detailViewController.mPlaylist_pos=i;
                }
            }
            
            detailViewController.mShouldUpdateInfos=1;
        }
        if (playlist->playlist_id) [self replacePlaylistDBwithCurrent];
        [self.tableView reloadData];
        
        
        if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) {
            currentPlayedEntry=detailViewController.mPlaylist_pos;
            NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
            int pos=currentPlayedEntry+1;
            if ((mDetailPlayerMode==0) && (integrated_playlist==0)) pos++;
            if (pos<[self.tableView numberOfRowsInSection:0]) [self.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:pos] animated:NO scrollPosition:UITableViewScrollPositionMiddle];
        }
        
        [self updateMiniPlayer];
    }
}


- (void)editPlaylist {
    if (playlist->nb_entries) {
        self.navigationItem.rightBarButtonItem = self.editButtonItem;
        [self setEditing:YES animated:YES];
    }
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        browse_depth=0;
        currentPlayedEntry=-1;
        mDetailPlayerMode=0;
        mFreePlaylist=1;
    }
    return self;
}

-(void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer {
    CGPoint p = [gestureRecognizer locationInView:self.tableView];
    
    NSIndexPath *indexPath = [self.tableView indexPathForRowAtPoint:p];
    if (indexPath != nil) {
        if ((gestureRecognizer.state==UIGestureRecognizerStateBegan)||(gestureRecognizer.state==UIGestureRecognizerStateChanged)) {
            int crow=indexPath.row;
            NSString *str=nil;
            if (show_playlist) {
                if (playlist->playlist_id==nil) crow++;
                crow-=2;
                str=playlist->entries[crow].fullpath;
            } else if (browse_depth>0) {
                t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
                str=cur_local_entries[crow].fullpath;
            }
            
            
            if (str) {
                //display popup
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

- (void)loadControllers {
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
    if (!self.detailViewController) self.detailViewController = [self findChildOfClass:[DetailViewControllerIphone class] inTabBarController:tbc];
}

- (void)viewDidLoad {
    START_PROFILE
    childController=NULL;

    if ([NSProcessInfo processInfo].isiOSAppOnMac) {
        self.hidesBottomBarWhenPushed = YES;
    } else if (@available(iOS 18.0, *)) {
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
            self.hidesBottomBarWhenPushed = YES;
        }
    }

    [self loadControllers];
    
    dictActionBtn=[NSMutableDictionary dictionaryWithCapacity:64];
    
    self.navigationController.delegate = self;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    mFileMngr=[[NSFileManager alloc] init];
    
    extractProgress = nil;
    is_rsn=0;
    
    mShowSubdir=0;
    
    if (browse_depth==0) mFreePlaylist=0;
    
    ratingImg[0] = @"heart-empty.png";
    ratingImg[1] = @"heart-half-filled.png";
    ratingImg[2] = @"heart-filled.png";
    /* Init popup view*/
    /**/
    
    //self.tableView.pagingEnabled;
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.tableView.sectionHeaderHeight = 18;
    self.tableView.rowHeight = 40;
    
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
    
    search_local=0;
    local_nb_entries=0;
    search_local_nb_entries=0;
    
    mSearchText=nil;
    mRenamePlaylist=0;
    mClickedPrimAction=0;
    list=nil;
    keys=nil;
    
    if (browse_depth==2) { //Playlist/Local mode
        currentPath = @"Documents";
        //[currentPath retain];
    }
    
    if (mDetailPlayerMode) self.navigationItem.rightBarButtonItem = nil;
    else {
        UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:NOW_PLAYING_ICON] style:UIBarButtonItemStylePlain target:self action:@selector(goPlayer)];
        self.navigationItem.rightBarButtonItem = item;
    }
    if (show_playlist) {
        sBar.frame=CGRectMake(0,0,0,0);
        sBar.hidden=TRUE;
        //[self.view setNeedsUpdateConstraints];
        //[self.view setNeedsLayout];
    }
    
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
    
    waitingViewExtract = [[WaitingView alloc] init];
    waitingViewExtract.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingViewExtract];
    waitingViewExtract.hidden=TRUE;
    
    views = NSDictionaryOfVariableBindings(waitingViewExtract);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingViewExtract(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingViewExtract(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingViewExtract attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingViewExtract attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
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
    
    [super viewDidLoad];
    
END_PROFILE
}

- (NSString*) getTitleFromTags:(NSString*)filePath {
    NSString *ret=[[filePath lastPathComponent] stringByDeletingPathExtension];
    NSURL *url = [NSURL fileURLWithPath:filePath];
    AVURLAsset *asset = [AVURLAsset URLAssetWithURL:url options:nil];
    // Métadonnées communes (recommandé)
    for (AVMetadataItem *item in asset.commonMetadata) {
        NSString *key = item.commonKey;
        NSString *value = item.stringValue;
        if (key && value) {
            if ([[key lowercaseString] isEqualToString:@"title"]) {
                ret=[NSString stringWithString:value];
                break;
            }
        }
    }
    NSNumber *track = nil;
    // ID3
    for (AVMetadataItem *item in [asset metadataForFormat:AVMetadataFormatID3Metadata]) {
        if ([item.key isEqual:AVMetadataID3MetadataKeyTrackNumber]||[item.key isEqual:@"TRK"]) {
            NSArray *parts = [item.stringValue componentsSeparatedByString:@"/"];
            track = @([parts.firstObject integerValue]);
            break;
        }
    }
    if (track==nil) {
        // iTunes
        for (AVMetadataItem *item in [asset metadataForFormat:AVMetadataFormatiTunesMetadata]) {
            if ([item.key isEqual:AVMetadataiTunesMetadataKeyTrackNumber]) {
                NSDictionary *dict = (NSDictionary *)item.value;
                track = dict[@"trackNumber"];
                break;
            }
        }
    }
    if (track==nil) {
        // Vorbis
        NSString *VorbisFormat = @"org.xiph.vorbis.comments";
        if ([asset.availableMetadataFormats containsObject:VorbisFormat]) {
            
            for (AVMetadataItem *item in [asset metadataForFormat:VorbisFormat]) {
                NSString *key = [item.key description];
                if ([key caseInsensitiveCompare:@"TRACKNUMBER"] == NSOrderedSame) {
                    track = @([item.stringValue integerValue]);
                    break;
                }
            }
        }
    }
    if (track) ret=[NSString stringWithFormat:@"%@.%@",track.stringValue,ret];
    return ret;
}

-(void) updatePlaylistEntriesFromTag {
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSString *ext;
    if (!playlist) return;
    for (int i=0;i<playlist->nb_entries;i++) {
        ext=[[playlist->entries[i].fullpath pathExtension] uppercaseString];
        for (NSString *item in filetype_extVGMSTREAM) {
            if ([item isEqualToString:ext]) {
                playlist->entries[i].label=[self getTitleFromTags:[ModizFileHelper getFullCleanFilePath:playlist->entries[i].fullpath]];
                break;
            }
        }
    }
}


-(void) fillKeys {
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    shouldFillKeys=1;
    search_local=0;
    
    if (browse_depth==0) {
        keys = [[NSMutableArray alloc] init];
        list = [[NSMutableArray alloc] init];
        NSMutableArray *mode_entries = [[NSMutableArray alloc] init];
        NSMutableArray *mode_entries_details = [[NSMutableArray alloc] init];
        [mode_entries addObject:NSLocalizedString(@"Add a playlist",@"")];
        [mode_entries_details addObject:NSLocalizedString(@"Create a new playlist",@"")];
        
        if (detailViewController.mPlaylist_size) {
            [mode_entries addObject:NSLocalizedString(@"Now playing",@"")];
            if (detailViewController.mPlaylist_size==1) [mode_entries_details addObject:NSLocalizedString(@"1 entry",@"")];
            else [mode_entries_details addObject:[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),detailViewController.mPlaylist_size]];
        }
        else {
            [mode_entries addObject:NSLocalizedString(@"Now playing",@"")];
            [mode_entries_details addObject:NSLocalizedString(@"No entry",@"")];
        }
        
        [mode_entries addObject:NSLocalizedString(@"Random picks",@"")];
        [mode_entries_details addObject:NSLocalizedString(@"Generate a randomized playlist",@"")];
        
        [mode_entries addObject:NSLocalizedString(@"Most played",@"")];
        [mode_entries_details addObject:[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),[self getMostPlayedCountFromDB]]];
        
        [mode_entries addObject:NSLocalizedString(@"Favorites",@"")];
        [mode_entries_details addObject:[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),[self getFavoritesCountFromDB]]];
        
        [self loadPlayListsListFromDB:mode_entries list_id:list entries_details:mode_entries_details];
        NSDictionary *mode_entriesDict = [NSDictionary dictionaryWithObjectsAndKeys:mode_entries,@"entries",mode_entries_details,@"entries_details", nil];
        //NSDictionary *mode_entries_detailsDict = [NSDictionary dictionaryWithObject:mode_entries_details forKey:@"entries_details"];
        [keys addObject:mode_entriesDict];
        //[keys addObject:mode_entries_detailsDict];
    } else if (show_playlist) {
        switch (integrated_playlist) {
            case 0: //not integrated playlist, refresh currently selected playlist
                [self loadPlayListsFromDB:playlist->playlist_id intoPlaylist:playlist];
                [self updatePlaylistEntriesFromTag];
                break;
            case INTEGRATED_PLAYLIST_NOWPLAYING:
                [self reloadNowPlaying];
                break;
            case INTEGRATED_PLAYLIST_FAVORITES:
                [self loadFavoritesList:playlist];
                break;
            case INTEGRATED_PLAYLIST_MOSTPLAYED:
                [self loadMostPlayedList:playlist];
                break;
            case INTEGRATED_PLAYLIST_RANDOM:
                //no real refresh needed
                break;
        }
    } else {
        //do not show playlist -> in browsing mode
        [self listLocalFiles];
    }
}

-(void) getStilInfo:(char*)fullPath {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    
    
    strcpy(browser_stil_info,"");
    pthread_mutex_lock(&db_mutexHVSCSTIL);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        char tmppath[256];
        sqlite3_stmt *stmt;
        char *realPath=strstr(fullPath,"/HVSC");
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        if (!realPath) {
            //try to find realPath with md5
            snprintf(sqlStatement,1024,"SELECT filepath FROM hvsc_path WHERE id_md5=\"%s\"",browser_song_md5);
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    strcpy(tmppath,(const char*)sqlite3_column_text(stmt, 0));
                    realPath=tmppath;
                }
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
        } else realPath+=5;
        if (realPath) {
            snprintf(sqlStatement,1024,"SELECT stil_info FROM stil WHERE fullpath=\"%s\"",realPath);
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    strcpy(browser_stil_info,(const char*)sqlite3_column_text(stmt, 0));
                    while ((realPath=strstr(browser_stil_info,"\\n"))) {
                        *realPath='\n';
                        realPath++;
                        memmove(realPath,realPath+1,strlen(realPath));
                    }
                }
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
        }
    };
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutexHVSCSTIL);
}


-(void) sid_parseStilInfo:(int)subsongs_nb {
    
    int idx=0;
    int parser_status=0;
    int parser_track_nb=0;
    int stil_info_len=strlen(browser_stil_info);
    char tmp_str[1024];
    int tmp_str_idx;
    
    browser_sidtune_name=(char**)calloc(subsongs_nb,sizeof(char*));
    
    browser_sidtune_title=(char**)calloc(subsongs_nb,sizeof(char*));
    
    while (browser_stil_info[idx]) {
        if ((browser_stil_info[idx]=='(')&&(browser_stil_info[idx+1]=='#')) {
            parser_status=1;
            parser_track_nb=0;
        }
        else {
            switch (parser_status) {
                case 1: // got a "(" before
                    if (browser_stil_info[idx]=='#') parser_status=2;
                    else parser_status=0;
                    break;
                case 2: // got a "(#" before
                    if ((browser_stil_info[idx]>='0')&&(browser_stil_info[idx]<='9')) {
                        parser_track_nb=parser_track_nb*10+(browser_stil_info[idx]-'0');
                        parser_status=2;
                    } else if (browser_stil_info[idx]==')') {
                        parser_status=3;
                    }
                    break;
                case 3: // got a "(#<track_nb>)" before
                    if (strncmp(browser_stil_info+idx,"NAME: ",strlen("NAME: "))==0) {
                        parser_status=4;
                        tmp_str[0]=0;
                        tmp_str_idx=0;
                        idx+=strlen("NAME: ")-1;
                    } else if (strncmp(browser_stil_info+idx,"TITLE: ",strlen("TITLE: "))==0) {
                        parser_status=5;
                        tmp_str[0]=0;
                        tmp_str_idx=0;
                        idx+=strlen("TITLE: ")-1;
                    } else
                        break;
                case 4: // "NAME: "
                    if (browser_stil_info[idx]==0x0A) {
                        parser_status=0;
                        if (parser_track_nb<=subsongs_nb) {
                            browser_sidtune_name[parser_track_nb-1]=(char*)malloc(tmp_str_idx+1);
                            strcpy(browser_sidtune_name[parser_track_nb-1],tmp_str);
                        }
                    } else {
                        tmp_str[tmp_str_idx]=browser_stil_info[idx];
                        if (tmp_str_idx<1024) tmp_str_idx++;
                        tmp_str[tmp_str_idx]=0;
                    }
                    break;
                case 5: // "TITLE: "
                    if (browser_stil_info[idx]==0x0A) {
                        parser_status=0;
                        if (parser_track_nb<=subsongs_nb) {
                            browser_sidtune_title[parser_track_nb-1]=(char*)malloc(tmp_str_idx+1);
                            strcpy(browser_sidtune_title[parser_track_nb-1],tmp_str);
                        }
                    } else {
                        tmp_str[tmp_str_idx]=browser_stil_info[idx];
                        if (tmp_str_idx<1024) tmp_str_idx++;
                        tmp_str[tmp_str_idx]=0;
                    }
                    break;
                default:
                    break;
            }
        }
        idx++;
        
    }
}

static void writeLEword(unsigned char ptr[2], int someWord) {
    ptr[0] = (someWord & 0xFF);
    ptr[1] = (someWord >> 8);
}



static void md5_from_buffer(char *dest, size_t destlen,char * buf, size_t bufsize)
{
    uint8_t md5[16];
    int ret;
    MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, (const unsigned char*)buf, bufsize);
    MD5Final(md5, &ctx);
    ret =
    snprintf(dest, destlen,
             "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
             md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6],
             md5[7], md5[8], md5[9], md5[10], md5[11], md5[12], md5[13],
             md5[14], md5[15]);
    if (ret >= destlen || ret != 32) {
        fprintf(stderr, "md5 buffer error (%d/%zd)\n", ret, destlen);
        exit(1);
    }
}

static int qsort_CompareArcEntries(const void *entryA, const void *entryB) {
    char *strA,*strB;
    int res;
    strA=*((char**)entryA);
    strB=*((char**)entryB);
    res=strcmp(strA,strB);
    if (res<0) return -1;
    if (res==0) return 0;
    return 1;
}


-(void) loadPlayListsListFromDB:(NSMutableArray*)entries list_id:(NSMutableArray*)list_id entries_details:(NSMutableArray*)details {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT id,name,num_files FROM playlists ORDER BY name");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                [entries addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)]];
                [list_id addObject:[NSString stringWithFormat:@"%d",sqlite3_column_int(stmt, 0)]];
                if (sqlite3_column_int(stmt, 2)==1) [details addObject:@"1 entry"];
                else [details addObject:[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),sqlite3_column_int(stmt, 2)]];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
}
-(void) loadPlayListsFromDB:(NSString *)_id_playlist intoPlaylist:(t_playlist *)_playlist  {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    
    pthread_mutex_lock(&db_mutex);
    _playlist->nb_entries=0;
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        
        //Get playlist name
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT id,name,num_files FROM playlists WHERE id=%s",[_id_playlist UTF8String]);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                _playlist->playlist_id=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                _playlist->playlist_name=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT p.name,p.fullpath,s.rating,s.play_count,s.avg_rating FROM playlists_entries p \
    LEFT OUTER JOIN user_stats s ON p.fullpath=s.fullpath \
    WHERE id_playlist=%s",[_id_playlist UTF8String]);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                _playlist->entries[_playlist->nb_entries].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                _playlist->entries[_playlist->nb_entries].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)];
                
                //adjust label
                if ([_playlist->entries[_playlist->nb_entries].fullpath rangeOfString:@"?"].location!=NSNotFound) {
                    _playlist->entries[_playlist->nb_entries].label=[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getFullCleanFilePath:[_playlist->entries[_playlist->nb_entries].fullpath lastPathComponent]] ,_playlist->entries[_playlist->nb_entries].label];
                } else {
                    
                    if ([_playlist->entries[_playlist->nb_entries].fullpath rangeOfString:@"@"].location!=NSNotFound) {
                        _playlist->entries[_playlist->nb_entries].label=[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getFullCleanFilePath:[_playlist->entries[_playlist->nb_entries].fullpath lastPathComponent]],_playlist->entries[_playlist->nb_entries].label];
                    }
                }
                
                signed char tmpsc=(signed char)sqlite3_column_int(stmt, 2);
                if (tmpsc<0) tmpsc=0;
                if (tmpsc>5) tmpsc=5;
                if ((tmpsc==0)&&(sqlite3_column_type(stmt,4)!=SQLITE_NULL)) {
                    tmpsc=(signed char)sqlite3_column_int(stmt, 4);
                    if (tmpsc) tmpsc=1;
                }
                _playlist->entries[_playlist->nb_entries].ratings=tmpsc;
                _playlist->entries[_playlist->nb_entries].playcounts=(short int)sqlite3_column_int(stmt, 3);
                _playlist->nb_entries++;
                if (_playlist->nb_entries>=MAX_PL_ENTRIES) break;
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
}
-(NSString *) getPlaylistNameDB:(NSString*)id_playlist {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    NSString *listName;
    sqlite3 *db;
    int err;
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //Get playlist name
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT name FROM playlists WHERE id=%s",[id_playlist UTF8String]);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                listName=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return listName;
}

-(bool) addListToPlaylistDB {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    bool result;
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        for (int i=0;i<playlist->nb_entries;i++) {
            snprintf(sqlStatement,sizeof(sqlStatement),"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
                     [playlist->playlist_id UTF8String],[[playlist->entries[i].label lastPathComponent] UTF8String],[playlist->entries[i].fullpath UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
                result=TRUE;
            } else {
                result=FALSE;
                MDZELog("ErrSQL : %d",err);
                break;
            }
        }
        if (result) {
            snprintf(sqlStatement,sizeof(sqlStatement),"UPDATE playlists SET num_files=\
     (SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
     WHERE id=%s",
                     [playlist->playlist_id UTF8String],[playlist->playlist_id UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
                result=TRUE;
            } else {
                result=FALSE;
                MDZELog("ErrSQL : %d",err);
            }
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return result;
}
-(bool) addListToPlaylistDB:(NSString*)id_playlist entries:(t_plPlaylist_entry*)pl_entries nb_entries:(int)nb_entries  {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    bool result;
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        for (int i=0;i<nb_entries;i++) {
            snprintf(sqlStatement,sizeof(sqlStatement),"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
                     [id_playlist UTF8String],[[pl_entries[i].mPlaylistFilename lastPathComponent] UTF8String],[pl_entries[i].mPlaylistFilepath UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
                result=TRUE;
            } else {
                result=FALSE;
                MDZELog("ErrSQL : %d",err);
                break;
            }
        }
        if (result) {
            snprintf(sqlStatement,sizeof(sqlStatement),"UPDATE playlists SET num_files=\
     (SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
     WHERE id=%s",
                     [id_playlist UTF8String],[id_playlist UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
                result=TRUE;
            } else {
                result=FALSE;
                MDZELog("ErrSQL : %d",err);
            }
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return result;
}
-(bool) removeFromPlaylistDB:(NSString*)id_playlist fullPath:(NSString*)fullpath {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    bool result;
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,sizeof(sqlStatement),"DELETE FROM playlists_entries WHERE id_playlist=\"%s\" AND fullpath=\"%s\"",
                 [id_playlist UTF8String],[fullpath UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
            result=TRUE;
        } else {
            MDZELog("ErrSQL : %d",err);
            result=FALSE;
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return result;
}
-(bool) replacePlaylistDBwithCurrent {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        bool allOK=true;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        err=sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);
        if (err==SQLITE_OK){
        } else {
            allOK=false;
            MDZELog("ErrSQL : %d",err);
        }
        
        if (allOK) {
            snprintf(sqlStatement,sizeof(sqlStatement),"DELETE FROM playlists_entries WHERE id_playlist=%s",
                     [playlist->playlist_id UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
            } else {
                allOK=false;
                MDZELog("ErrSQL : %d",err);
            }
        }
        
        if (allOK) {
            for (int i=0;i<playlist->nb_entries;i++) {
                snprintf(sqlStatement,sizeof(sqlStatement),"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
                         [playlist->playlist_id UTF8String],[[playlist->entries[i].label lastPathComponent] UTF8String],[playlist->entries[i].fullpath UTF8String]);
                err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
                if (err==SQLITE_OK){
                } else {
                    allOK=false;
                    MDZELog("ErrSQL : %d",err);
                }
            }
        }
        
        if (allOK) {
            err=sqlite3_exec(db, "COMMIT", 0, 0, 0);
            if (err==SQLITE_OK){
            } else MDZELog("ErrSQL : %d",err);
        } else {
            err=sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
            if (err==SQLITE_OK){
            } else MDZELog("ErrSQL : %d",err);
        }
        
        snprintf(sqlStatement,sizeof(sqlStatement),"UPDATE playlists SET num_files=\
    (SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
    WHERE id=%s",
                 [playlist->playlist_id UTF8String],[playlist->playlist_id UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return TRUE;
}
-(void) updatePlaylistNameDB:(NSString*)id_playlist playlist_name:(NSString *)playlist_name {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,sizeof(sqlStatement),"UPDATE playlists SET name=\"%s\" WHERE id=%s",[playlist_name UTF8String],[id_playlist UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
}

-(int) deletePlaylistDB:(NSString*)id_playlist {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err,ret;
    pthread_mutex_lock(&db_mutex);
    ret=1;
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        
        snprintf(sqlStatement,sizeof(sqlStatement),"DELETE FROM playlists_entries WHERE id_playlist=%s",[id_playlist UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else {ret=0;MDZELog("ErrSQL : %d",err);}
        
        snprintf(sqlStatement,sizeof(sqlStatement),"DELETE FROM playlists WHERE id=%s",[id_playlist UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else {ret=0;MDZELog("ErrSQL : %d",err);}
        
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return ret;
}

-(void) listLocalFiles {
    NSString *file,*cpath;
    NSURL *fileURL;
    NSMutableArray *filetype_ext=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE];
    NSMutableArray *filetype_extAMIGA=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE_AMIGA];
    NSMutableArray *all_multisongstype_ext=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE_SUBSONGS];
    NSMutableArray *archivetype_ext=[ModizFileHelper buildListSupportFileType:FTYPE_ARCHIVE];
    
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    char sqlStatement[1024];
    sqlite3_stmt *stmt;
    int local_entries_index,local_nb_entries_limit;
    int browseType;
    int shouldStop=0;
    static bool no_reentrant=false;
    
    if (no_reentrant) return;
    no_reentrant=true;
    
    // First check count for each section
    cpath=[ModizFileHelper getFullPathForFilePath:currentPath];
    //Check if it is a directory or an archive
    BOOL isDirectory;
    browseType=0;
    if ([mFileMngr fileExistsAtPath:cpath isDirectory:&isDirectory]) {
        if (!isDirectory) {
            //file:check if archive or multisongs
            NSString *extension=[[[cpath lastPathComponent] pathExtension] uppercaseString];
            if ([archivetype_ext indexOfObject:extension]!=NSNotFound) {
                //check if really an archive
                if ([ModizFileHelper isABrowsableArchive:cpath]) browseType=1;
            }
            //check if Multisongs file
            else if ([all_multisongstype_ext indexOfObject:extension]!=NSNotFound) {
                //check if really a gme file
                if ([ModizFileHelper isGMEFileWithSubsongs:cpath]) browseType=2;
                else if ([ModizFileHelper isSidFileWithSubsongs:cpath]) browseType=3;
            }
        }
    }
    
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    search_local=0;
    
    if (search_local_nb_entries) {
        for (int j=0;j<search_local_entries_count;j++) {
            search_local_entries[j].label=nil;
            search_local_entries[j].altlabel=nil;
            search_local_entries[j].fullpath=nil;
        }
        search_local_entries=NULL;
        search_local_nb_entries=0;
        free(search_local_entries_data);
    }
    
    if (mSearch) {
        search_local=1;
        
        search_local_entries_data=(t_local_browse_entry*)calloc(local_nb_entries,sizeof(t_local_browse_entry));
        
        search_local_entries_count=0;
        if (local_entries_count) search_local_entries=search_local_entries_data;
        for (int j=0;j<local_entries_count;j++)  {
            
            bool found=false;
            if ((browseType==0)&&mShowSubdir) {
                found=[self searchStringRegExp:mSearchText sourceString:local_entries[j].label];
                if (!found) found=[self searchStringRegExp:mSearchText sourceString:local_entries[j].fullpath];
            } else {
                found=[self searchStringRegExp:mSearchText sourceString:local_entries[j].label];
            }
            
            if  (found ||([mSearchText length]==0)) {
                search_local_entries[search_local_entries_count].label=local_entries[j].label;
                search_local_entries[search_local_entries_count].altlabel=local_entries[j].altlabel;
                search_local_entries[search_local_entries_count].fullpath=local_entries[j].fullpath;
                search_local_entries[search_local_entries_count].playcount=local_entries[j].playcount;
                search_local_entries[search_local_entries_count].rating=local_entries[j].rating;
                search_local_entries[search_local_entries_count].type=local_entries[j].type;
                
                search_local_entries[search_local_entries_count].song_length=local_entries[j].song_length;
                search_local_entries[search_local_entries_count].songs=local_entries[j].songs;
                search_local_entries[search_local_entries_count].channels_nb=local_entries[j].channels_nb;
                
                search_local_entries_count++;
                search_local_nb_entries++;
            }
        }
        no_reentrant=false;
        return;
    }
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) != SQLITE_OK) db=NULL;
    
    err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
    if (err==SQLITE_OK){
    } else MDZELog("ErrSQL : %d",err);
    
    
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {
            local_entries_data[i].label=nil;
            local_entries_data[i].altlabel=nil;
            local_entries_data[i].fullpath=nil;
        }
        for (int j=0;j<local_entries_count;j++) {
            local_entries[j].label=nil;
            local_entries[j].altlabel=nil;
            local_entries[j].fullpath=nil;
        }
        local_entries=NULL;
        free(local_entries_data);local_entries_data=NULL;
        local_nb_entries=0;
    }
    local_entries_count=0;
    
    
    
    if (browseType==3) {//SID
        SidTune *mSidTune=new SidTune([cpath UTF8String],0,true);
        bool sid_ok=true;
        if (mSidTune==NULL) sid_ok=false;
        else if (!(mSidTune->getStatus())) sid_ok=false;
        if (!sid_ok) {
            MDZELog("SID SidTune init error");
            if (mSidTune) {delete mSidTune;mSidTune=NULL;}
        } else {
            const SidTuneInfo *sidtune_info;
            sidtune_info=mSidTune->getInfo();
            
            
            
            //Compute MD5
            memset(browser_song_md5,0,33);
            
            mSidTune->createMD5New(browser_song_md5);
            browser_song_md5[32]=0;
            
            
            //Get STIL info
            browser_stil_info=(char*)calloc(1,MAX_STIL_DATA_LENGTH);
            [self getStilInfo:(char*)[cpath UTF8String]];
            
            [self sid_parseStilInfo:sidtune_info->songs()];
            mdz_safe_free(browser_stil_info)
            
            for (int i=0;i<sidtune_info->songs();i++){
                const SidTuneInfo *s_info;
                file=nil;
                mSidTune->selectSong(i);
                s_info=mSidTune->getInfo();
                
                if (browser_sidtune_name[i]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,browser_sidtune_name[i]];
                else if (browser_sidtune_title[i]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,browser_sidtune_title[i]];
                else if (s_info->infoString(0)[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,s_info->infoString(0)];
                else file=[NSString stringWithFormat:@"%.3d-%@",i+1,[cpath lastPathComponent]];
                
                int filtered=0;
                if ((mSearch)&&([mSearchText length]>0)) {
                    filtered=1;
                    //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                    //if (r.location != NSNotFound) {
                    if ([self searchStringRegExp:mSearchText sourceString:file]) {
                        /*if(r.location== 0)*/ filtered=0;
                    }
                }
                if (!filtered) {
                    local_entries_count++;
                    local_nb_entries++;
                }
            }
            
            if (local_nb_entries) {
                //2nd initialize array to receive entries
                local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries,sizeof(t_local_browse_entry));
                if (!local_entries_data) {
                    //Not enough memory
                    //try to allocate less entries
                    local_nb_entries_limit=LIMITED_LIST_SIZE;
                    if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                    local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries_limit,sizeof(t_local_browse_entry));
                    if (local_entries_data==NULL) {
                        //show alert : cannot list
                        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem.",@"")];
                    } else {
                        //show alert : limited list
                        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                        local_nb_entries=local_nb_entries_limit;
                    }
                } else local_nb_entries_limit=0;
                if (local_entries_data) {
                    local_entries_index=0;
                    if (local_entries_count) {
                        if (local_entries_index+local_entries_count>local_nb_entries) {
                            local_entries_count=local_nb_entries-local_entries_index;
                            local_entries=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count;
                            local_entries_count=0;
                        } else {
                            local_entries=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count;
                            local_entries_count=0;
                        }
                    }
                    
                    for (int i=0;i<sidtune_info->songs();i++){
                        const SidTuneInfo *s_info;
                        file=nil;
                        mSidTune->selectSong(i);
                        s_info=mSidTune->getInfo();
                        
                        if (browser_sidtune_name[i]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,browser_sidtune_name[i]];
                        else if (browser_sidtune_title[i]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,browser_sidtune_title[i]];
                        else if (s_info->infoString(0)[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,s_info->infoString(0)];
                        else file=[NSString stringWithFormat:@"%.3d-%@",i+1,[cpath lastPathComponent]];
                        
                        int filtered=0;
                        if ((mSearch)&&([mSearchText length]>0)) {
                            filtered=1;
                            //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                            //if (r.location != NSNotFound) {
                            if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                /*if(r.location== 0)*/ filtered=0;
                            }
                        }
                        
                        if (!filtered) {
                            
                            local_entries[local_entries_count].type=1;
                            local_entries[local_entries_count].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                            local_entries[local_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@?%d",currentPath,i];
                            
                            local_entries[local_entries_count].rating=0;
                            local_entries[local_entries_count].playcount=0;
                            local_entries[local_entries_count].song_length=0;
                            local_entries[local_entries_count].songs=1;//0;
                            local_entries[local_entries_count].channels_nb=0;
                            
                            snprintf(sqlStatement,1024,"SELECT play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[local_entries[local_entries_count].fullpath UTF8String]);
                            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                            if (err==SQLITE_OK){
                                while (sqlite3_step(stmt) == SQLITE_ROW) {
                                    signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                    if (rating<0) rating=0;
                                    if (rating>5) rating=5;
                                    if ((rating==0)&&(sqlite3_column_type(stmt,5)!=SQLITE_NULL)) {
                                        rating=(signed char)sqlite3_column_int(stmt, 5);
                                        if (rating) rating=1;
                                    }
                                    local_entries[local_entries_count].playcount=(short int)sqlite3_column_int(stmt, 0);
                                    local_entries[local_entries_count].rating=rating;
                                    local_entries[local_entries_count].song_length=(int)sqlite3_column_int(stmt, 2);
                                    local_entries[local_entries_count].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                }
                                sqlite3_finalize(stmt);
                            } else MDZELog("ErrSQL : %d",err);
                            
                            local_entries_count++;
                            
                            if (local_nb_entries_limit) {
                                local_nb_entries_limit--;
                                if (!local_nb_entries_limit) shouldStop=1;
                            }
                            
                        }
                    }
                }
            }
            if (browser_sidtune_title) {
                for (int i=0;i<sidtune_info->songs();i++)
                    if (browser_sidtune_title[i]) free(browser_sidtune_title[i]);
                free(browser_sidtune_title);
                browser_sidtune_title=NULL;
            }
            if (browser_sidtune_name) {
                for (int i=0;i<sidtune_info->songs();i++)
                    if (browser_sidtune_name[i]) free(browser_sidtune_name[i]);
                free(browser_sidtune_name);
                browser_sidtune_name=NULL;
            }
            if (mSidTune) {delete mSidTune;mSidTune=NULL;}
        }
    } else if (browseType==2) { //GME Multisongs
        // Open music file in new emulator
        Music_Emu* gme_emu;
        gme_emu=NULL;
        gme_err_t gme_err=gme_open_file( [cpath UTF8String], &gme_emu, gme_info_only );
        if (gme_err) {
            MDZELog("gme_open_file error: %s",gme_err);
        } else {
            gme_info_t *gme_info;
            
            //is a m3u available ?
            NSString *tmpStr=[NSString stringWithFormat:@"%@.m3u",[cpath stringByDeletingPathExtension]];
            gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String],[[cpath lastPathComponent] UTF8String] );
            if (gme_err) {
                NSString *tmpStr=[NSString stringWithFormat:@"%@.M3U",[cpath stringByDeletingPathExtension]];
                gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String],[[cpath lastPathComponent] UTF8String] );
            }
            
            
            int total_trackNb=gme_track_count( gme_emu );
            for (int i=0;i<total_trackNb;i++) {
                //err=gme_start_track( gme_emu, i );
                //if (!err) {
                if (gme_track_info( gme_emu, &gme_info, i )==0) {
                    file=nil;
                    if (gme_info->song) {
                        if (gme_info->song[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,gme_info->song];
                    }
                    if (!file) {
                        if (gme_info->game) {
                            if (gme_info->game[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,gme_info->game];
                        }
                    }
                    if (!file) {
                        file=[NSString stringWithFormat:@"%.3d-%@",i+1,[cpath lastPathComponent]];
                    }
                    
                    int filtered=0;
                    if ((mSearch)&&([mSearchText length]>0)) {
                        filtered=1;
                        //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                        //if (r.location != NSNotFound) {
                        if ([self searchStringRegExp:mSearchText sourceString:file]) {
                            /*if(r.location== 0)*/ filtered=0;
                        }
                    }
                    if (!filtered) {
                        local_entries_count++;
                        local_nb_entries++;
                    }
                    gme_free_info(gme_info);
                }
                //}
            }
            gme_delete(gme_emu);
        }
        if (local_nb_entries) {
            //2nd initialize array to receive entries
            local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries,sizeof(t_local_browse_entry));
            if (!local_entries_data) {
                //Not enough memory
                //try to allocate less entries
                local_nb_entries_limit=LIMITED_LIST_SIZE;
                if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries_limit,sizeof(t_local_browse_entry));
                if (local_entries_data==NULL) {
                    //show alert : cannot list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem.",@"")];
                } else {
                    //show alert : limited list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                if (local_entries_count) {
                    if (local_entries_index+local_entries_count>local_nb_entries) {
                        local_entries_count=local_nb_entries-local_entries_index;
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    } else {
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    }
                }
                
                gme_err_t gme_err=gme_open_file( [cpath UTF8String], &gme_emu, gme_info_only );
                if (gme_err) {
                    MDZELog("gme_open_file error: %s",gme_err);
                } else {
                    gme_info_t *gme_info;
                    
                    //is a m3u available ?
                    NSString *tmpStr=[NSString stringWithFormat:@"%@.m3u",[cpath stringByDeletingPathExtension]];
                    gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String],[[cpath lastPathComponent] UTF8String] );
                    if (gme_err) {
                        NSString *tmpStr=[NSString stringWithFormat:@"%@.M3U",[cpath stringByDeletingPathExtension]];
                        gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String],[[cpath lastPathComponent] UTF8String] );
                    }
                    
                    for (int i=0;i<gme_track_count( gme_emu );i++) {
                        if (gme_track_info( gme_emu, &gme_info, i )==0) {
                            file=nil;
                            if (gme_info->song) {
                                if (gme_info->song[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,gme_info->song];
                            }
                            if (!file) {
                                if (gme_info->game) {
                                    if (gme_info->game[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,gme_info->game];
                                }
                            }
                            if (!file) {
                                file=[NSString stringWithFormat:@"%.3d-%@",i+1,[cpath lastPathComponent]];
                            }
                            
                            int filtered=0;
                            if ((mSearch)&&([mSearchText length]>0)) {
                                filtered=1;
                                //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                    //if (r.location != NSNotFound) {
                                    /*if(r.location== 0)*/ filtered=0;
                                }
                            }
                            if (!filtered) {
                                
                                local_entries[local_entries_count].type=1;
                                local_entries[local_entries_count].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                                local_entries[local_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@?%d",currentPath,i];
                                
                                local_entries[local_entries_count].rating=0;
                                local_entries[local_entries_count].playcount=0;
                                local_entries[local_entries_count].song_length=0;
                                local_entries[local_entries_count].songs=1;//0;
                                local_entries[local_entries_count].channels_nb=0;
                                
                                snprintf(sqlStatement,1024,"SELECT play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[local_entries[local_entries_count].fullpath UTF8String]);
                                err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                                if (err==SQLITE_OK){
                                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                                        signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                        if (rating<0) rating=0;
                                        if (rating>5) rating=5;
                                        if ((rating==0)&&(sqlite3_column_type(stmt,5)!=SQLITE_NULL)) {
                                            rating=(signed char)sqlite3_column_int(stmt, 5);
                                            if (rating) rating=1;
                                        }
                                        local_entries[local_entries_count].playcount=(short int)sqlite3_column_int(stmt, 0);
                                        local_entries[local_entries_count].rating=rating;
                                        local_entries[local_entries_count].song_length=(int)sqlite3_column_int(stmt, 2);
                                        local_entries[local_entries_count].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                    }
                                    sqlite3_finalize(stmt);
                                } else MDZELog("ErrSQL : %d",err);
                                
                                local_entries_count++;
                                
                                if (local_nb_entries_limit) {
                                    local_nb_entries_limit--;
                                    if (!local_nb_entries_limit) shouldStop=1;
                                }
                                
                            }
                            gme_free_info(gme_info);
                        }
                    }
                    gme_delete(gme_emu);
                }
            }
        }
    } else if (browseType==1) { //Archive
        char **archive_entries;
        int archive_entries_count;
        int found=[ModizFileHelper scanarchive:[cpath UTF8String] filesList_ptr:&archive_entries filesCount_ptr:&archive_entries_count];
        int file_idx=0;
        
        if (found) {
            
            //sort the file list
            qsort(archive_entries, archive_entries_count, sizeof(char*), &qsort_CompareArcEntries);
            
            
            NSString *extension=[[[cpath lastPathComponent] pathExtension] uppercaseString];
            if (!is_rsn && ([extension caseInsensitiveCompare:@"rsn"]==NSOrderedSame)) {
                is_rsn=1;
                
                dispatch_async(dispatch_get_main_queue(), ^(void){
                    //Run UI Updates
                    [self.tableView setUserInteractionEnabled:false];
                    [self.navigationItem setHidesBackButton:YES animated:YES];
                });
                
                extractProgress = [NSProgress progressWithTotalUnitCount:1];
                extractProgress.cancellable = YES;
                extractProgress.pausable = NO;
                NSString *tmpPath=[NSString stringWithFormat:@"%@/tmpArchiveBrowser",NSTemporaryDirectory()];
                [mFileMngr removeItemAtPath:tmpPath error:NULL];
                [ModizFileHelper extractToPath:[cpath UTF8String] path:[tmpPath UTF8String] caller:self progress:extractProgress context:ExtractBrowserListProgressObserverContext completion:nil];
            }
            
            while (file_idx<found) {
                
                file=[NSString stringWithUTF8String:archive_entries[file_idx]];
                
                NSString *extension;
                NSString *file_no_ext;
                
                NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
                extension = (NSString *)[temparray_filepath lastObject];
                file_no_ext=[temparray_filepath firstObject];
                
                int filtered=0;
                if ((mSearch)&&([mSearchText length]>0)) {
                    filtered=1;
                    if ([self searchStringRegExp:mSearchText sourceString:file]) {
                        filtered=0;
                    }
                }
                if (!filtered) {
                    int found=0;
                    
                    if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                    else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                    
                    if (found)  {
                        local_entries_count++;
                        local_nb_entries++;
                    }
                }
                file_idx++;
            }
        } else {
        }
        
        if (local_nb_entries) {
            //2nd initialize array to receive entries
            local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries,sizeof(t_local_browse_entry));
            if (!local_entries_data) {
                //Not enough memory
                //try to allocate less entries
                local_nb_entries_limit=LIMITED_LIST_SIZE;
                if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries_limit,sizeof(t_local_browse_entry));
                if (local_entries_data==NULL) {
                    //show alert : cannot list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem.",@"")];
                } else {
                    //show alert : limited list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                if (local_entries_count) {
                    if (local_entries_index+local_entries_count>local_nb_entries) {
                        local_entries_count=local_nb_entries-local_entries_index;
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    } else {
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    }
                }
                
                file_idx=0;
                if (found) {
                    int arc_counter=0;
                    //while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
                    while (file_idx<found) {
                        //file=[ModizFileHelper getCorrectFileName:[cpath UTF8String] archive:a entry:entry];
                        file=[NSString stringWithUTF8String:archive_entries[file_idx]];
                        
                        NSString *extension;// = [[file pathExtension] uppercaseString];
                        NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                        
                        NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
                        extension = (NSString *)[temparray_filepath lastObject];
                        file_no_ext=[temparray_filepath firstObject];
                        
                        
                        int filtered=0;
                        if ((mSearch)&&([mSearchText length]>0)) {
                            filtered=1;
                            if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                filtered=0;
                            }
                        }
                        if (!filtered) {
                            int found=0;
                            
                            if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                            else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                            
                            if (found)  {
                                const char *str;
                                char tmp_str[1024];//,*tmp_convstr;
                                int other_encoding=0;
                                str=[[file lastPathComponent] UTF8String];
                                if ([extension caseInsensitiveCompare:@"mdx"]==NSOrderedSame ) {
                                    [[file lastPathComponent] getFileSystemRepresentation:tmp_str maxLength:1024];
                                    //tmp_convstr=mdx_make_sjis_to_syscharset(tmp_str);
                                    other_encoding=1;
                                }
                                local_entries[local_entries_count].type=1;
                                //NOT Supported: archive or multisongs in an archive
                                
                                if (other_encoding) {
                                    local_entries[local_entries_count].label=[[NSString alloc ] initWithCString:tmp_str encoding:NSUTF8StringEncoding];
                                    //    free(tmp_convstr);
                                } else local_entries[local_entries_count].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                                
                                local_entries[local_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@@%d",currentPath,arc_counter];
                                
                                local_entries[local_entries_count].rating=0;
                                local_entries[local_entries_count].playcount=0;
                                local_entries[local_entries_count].song_length=0;
                                local_entries[local_entries_count].songs=0;
                                local_entries[local_entries_count].channels_nb=0;
                                
                                snprintf(sqlStatement,1024,"SELECT play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[local_entries[local_entries_count].fullpath UTF8String]);
                                err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                                if (err==SQLITE_OK){
                                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                                        signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                        if (rating<0) rating=0;
                                        if (rating>5) rating=5;
                                        if ((rating==0)&&(sqlite3_column_type(stmt,5)!=SQLITE_NULL)) {
                                            rating=(signed char)sqlite3_column_int(stmt, 5);
                                            if (rating) rating=1;
                                        }
                                        local_entries[local_entries_count].playcount=(short int)sqlite3_column_int(stmt, 0);
                                        local_entries[local_entries_count].rating=rating;
                                        local_entries[local_entries_count].song_length=(int)sqlite3_column_int(stmt, 2);
                                        local_entries[local_entries_count].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                        local_entries[local_entries_count].songs=(int)sqlite3_column_int(stmt, 4);
                                    }
                                    sqlite3_finalize(stmt);
                                } else MDZELog("ErrSQL : %d",err);
                                
                                local_entries_count++;
                                arc_counter++;
                                
                                if (local_nb_entries_limit) {
                                    local_nb_entries_limit--;
                                    if (!local_nb_entries_limit) shouldStop=1;
                                }
                            }
                        }
                        file_idx++;
                    }
                }
            }
            
        }
    } else {
        clock_t start_time,end_time;
        start_time=clock();
        NSError *error;
        NSRange rdir;
        NSArray *dirContent;//=[NSMutableArray array];//
        BOOL isDir;
        
        //List all entries
        NSURL *directoryURL = [NSURL fileURLWithPath:cpath];
        NSDirectoryEnumerator *directoryEnumerator =
        [mFileMngr enumeratorAtURL:directoryURL
        includingPropertiesForKeys:@[NSURLPathKey, NSURLNameKey, NSURLIsDirectoryKey]
                           options:NSDirectoryEnumerationSkipsHiddenFiles|(mShowSubdir?0:NSDirectoryEnumerationSkipsSubdirectoryDescendants)
                      errorHandler:nil];
        
        /*for (NSURL *fileURL in directoryEnumerator) {
         [dirContent addObject:fileURL];
         }*/
        dirContent=[directoryEnumerator allObjects];
        
        //if (mShowSubdir) dirContent=[mFileMngr subpathsOfDirectoryAtPath:cpath error:&error];
        //else dirContent=[mFileMngr contentsOfDirectoryAtPath:cpath error:&error];
        
        //NSArray *sortedDirContent = [dirContent sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
        NSArray *sortedDirContent = [dirContent sortedArrayUsingComparator:^(id obj1, id obj2) {
            
            NSString *str1;//[(NSString *)obj1 lastPathComponent];
            NSString *str2;//[(NSString *)obj2 lastPathComponent];
            
            if (mShowSubdir==2) { //use path
                [(NSURL*)obj1 getResourceValue:&str1 forKey:NSURLPathKey error:nil];
                [(NSURL*)obj2 getResourceValue:&str2 forKey:NSURLPathKey error:nil];
            } else { //use filename
                [(NSURL*)obj1 getResourceValue:&str1 forKey:NSURLNameKey error:nil];
                [(NSURL*)obj2 getResourceValue:&str2 forKey:NSURLNameKey error:nil];
            }
            return [str1 caseInsensitiveCompare:str2];
        }];
        
        int file_idx=0;
        int file_cnt=[sortedDirContent count];
        for (fileURL in sortedDirContent) {
            //[mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
            NSNumber *isDirectory = nil;
            [fileURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];
            [fileURL getResourceValue:&file forKey:NSURLPathKey error:nil];
            
            
            rdir=[file rangeOfString:cpath];
            if (rdir.location!=NSNotFound) {
                file=[file substringFromIndex:(rdir.location+rdir.length+1)];
            }
            
            isDir=[isDirectory boolValue];
            
            if (isDir) { //rdir.location == NSNotFound) {  //assume it is a dir if no "." in file name
                if (![file isEqualToString:@"ProjectM"]) {
                    rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                    if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                        if (1/*[file compare:@"tmpArchive"]!=NSOrderedSame*/) {
                            //do not display dir if subdir mode is on
                            int filtered=mShowSubdir;
                            if (!filtered) {
                                if ((mSearch)&&([mSearchText length]>0)) {
                                    filtered=1;
                                    //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                    //if (r.location != NSNotFound) {
                                    if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                        /*if(r.location== 0)*/ filtered=0;
                                    }
                                }
                                if (!filtered) {
                                    local_entries_count++;
                                    local_nb_entries++;
                                }
                            }
                        }
                    }
                }
            } else {
                rdir.location=NSNotFound;
                rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                    NSString *extension;// = [[file pathExtension] uppercaseString];
                    NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
                    extension = (NSString *)[temparray_filepath lastObject];
                    //[temparray_filepath removeLastObject];
                    file_no_ext=[temparray_filepath firstObject];
                    
                    
                    int filtered=0;
                    if ((mSearch)&&([mSearchText length]>0)) {
                        filtered=1;
                        //NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                        //if (r.location != NSNotFound) {
                        if ([self searchStringRegExp:mSearchText sourceString:file]) {
                            /*if(r.location== 0)*/ filtered=0;
                        }
                    }
                    if (!filtered) {
                        int found=0;
                        if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                        else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                        else if ([archivetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                        
                        if (found)  {
                            local_entries_count++;
                            local_nb_entries++;
                        }
                    }
                }
            }
        }
        
        if (local_nb_entries) {
            //2nd initialize array to receive entries
            local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries,sizeof(t_local_browse_entry));
            
            if (!local_entries_data) {
                //Not enough memory
                //try to allocate less entries
                local_nb_entries_limit=LIMITED_LIST_SIZE;
                if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries_limit,sizeof(t_local_browse_entry));
                if (local_entries_data==NULL) {
                    //show alert : cannot list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem.",@"")];
                } else {
                    //show alert : limited list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                if (local_entries_count) {
                    if (local_entries_index+local_entries_count>local_nb_entries) {
                        local_entries_count=local_nb_entries-local_entries_index;
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    } else {
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    }
                }
                
                // Second check count for each section
                for (fileURL in sortedDirContent) {
                    if (shouldStop) break;
                    NSNumber *isDirectory = nil;
                    [fileURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];
                    [fileURL getResourceValue:&file forKey:NSURLPathKey error:nil];
                    rdir=[file rangeOfString:cpath];
                    if (rdir.location!=NSNotFound) {
                        file=[file substringFromIndex:(rdir.location+rdir.length+1)];
                    }
                    isDir=[isDirectory boolValue];
                    
                    
                    if (isDir) { //rdir.location == NSNotFound) {  //assume it is a dir if no "." in file name
                        if (![file isEqualToString:@"ProjectM"]) {
                            rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                            if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                                if (1/*[file compare:@"tmpArchive"]!=NSOrderedSame*/) {
                                    //do not display dir if subdir mode is on
                                    int filtered=mShowSubdir;
                                    if (!filtered) {
                                        if ((mSearch)&&([mSearchText length]>0)) {
                                            filtered=1;
                                            //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                            //if (r.location != NSNotFound) {
                                            if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                                /*if(r.location== 0)*/ filtered=0;
                                            }
                                        }
                                        if (!filtered) {
                                            local_entries[local_entries_count].type=0;
                                            
                                            local_entries[local_entries_count].label=[[NSString alloc] initWithString:file];
                                            
                                            local_entries[local_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@/%@",currentPath,file];
                                            local_entries_count++;
                                            if (local_nb_entries_limit) {
                                                local_nb_entries_limit--;
                                                if (!local_nb_entries_limit) shouldStop=1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        rdir.location=NSNotFound;
                        rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                        if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                            NSString *extension;// = [[file pathExtension] uppercaseString];
                            NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                            
                            NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
                            extension = (NSString *)[temparray_filepath lastObject];
                            //[temparray_filepath removeLastObject];
                            file_no_ext=[temparray_filepath firstObject];
                            
                            int filtered=0;
                            if ((mSearch)&&([mSearchText length]>0)) {
                                filtered=1;
                                //NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                //if (r.location != NSNotFound) {
                                if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                    /*if(r.location== 0)*/ filtered=0;
                                }
                            }
                            if (!filtered) {
                                int found=0;
                                
                                if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                                else if ([filetype_extAMIGA indexOfObject:file_no_ext]!=NSNotFound) found=1;
                                else if ([archivetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                                
                                
                                if (found)  {
                                    char tmp_str[1024];//,*tmp_convstr;
                                    int other_encoding=0;
                                    
                                    if ([extension caseInsensitiveCompare:@"mdx"]==NSOrderedSame ) {
                                        [[file lastPathComponent] getFileSystemRepresentation:tmp_str maxLength:1024];
                                        //tmp_convstr=mdx_make_sjis_to_syscharset(tmp_str);
                                        other_encoding=1;
                                    }
                                    local_entries[local_entries_count].type=1;
                                    //check if Archive file
                                    if ([archivetype_ext indexOfObject:extension]!=NSNotFound) { //check if really an archive
                                        local_entries[local_entries_count].type=2|16;  //16 is to flag them as to check before displaying entry in tabiew
                                    } else if ([all_multisongstype_ext indexOfObject:extension]!=NSNotFound) { //check if Multisongs file
                                        local_entries[local_entries_count].type=3|16;  //16 is to flag them as to check before displaying entry in tabiew
                                        
                                    }
                                    
                                    if (other_encoding) {
                                        local_entries[local_entries_count].label=[[NSString alloc] initWithCString:tmp_str encoding:NSUTF8StringEncoding];
                                        //    free(tmp_convstr);
                                    } else local_entries[local_entries_count].label=[[NSString alloc] initWithString:[file lastPathComponent]];
                                    
                                    local_entries[local_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@/%@",currentPath,file];
                                    
                                    local_entries[local_entries_count].rating=-1;
                                    local_entries[local_entries_count].playcount=-1;
                                    local_entries[local_entries_count].song_length=-1;
                                    local_entries[local_entries_count].songs=-1;
                                    local_entries[local_entries_count].channels_nb=-1;
                                    
                                    local_entries_count++;
                                    
                                    if (local_nb_entries_limit) {
                                        local_nb_entries_limit--;
                                        if (!local_nb_entries_limit) shouldStop=1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (db) {
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
    }
    
    no_reentrant=false;
    return;
}

-(void) loadFavoritesList:(t_playlist*)playlist {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    
    playlist->nb_entries=0;
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT name,fullpath,rating,play_count,length,channels,songs FROM user_stats WHERE rating=5 ORDER BY rating DESC,name");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                playlist->entries[playlist->nb_entries].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                playlist->entries[playlist->nb_entries].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)];
                
                //adjust label
                if ([playlist->entries[playlist->nb_entries].fullpath rangeOfString:@"?"].location!=NSNotFound) {
                    playlist->entries[playlist->nb_entries].label=[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getFullCleanFilePath:[playlist->entries[playlist->nb_entries].fullpath lastPathComponent]] ,playlist->entries[playlist->nb_entries].label];
                } else {
                    
                    if ([playlist->entries[playlist->nb_entries].fullpath rangeOfString:@"@"].location!=NSNotFound) {
                        playlist->entries[playlist->nb_entries].label=[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getFullCleanFilePath:[playlist->entries[playlist->nb_entries].fullpath lastPathComponent]],playlist->entries[playlist->nb_entries].label];
                    }
                }
                
                playlist->entries[playlist->nb_entries].ratings=(signed char)sqlite3_column_int(stmt,2);
                
                playlist->entries[playlist->nb_entries].playcounts=(short int)sqlite3_column_int(stmt,3);
                playlist->entries[playlist->nb_entries].song_length=(int)sqlite3_column_int(stmt,4);
                playlist->entries[playlist->nb_entries].channels_nb=(char)sqlite3_column_int(stmt,5);
                playlist->entries[playlist->nb_entries].songs=(int)sqlite3_column_int(stmt,6);
                playlist->nb_entries++;
                if (playlist->nb_entries>=MAX_PL_ENTRIES) {
                    MDZELog("max entries reached (%d)",MAX_PL_ENTRIES);
                    break;
                }
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
}

-(void) loadMostPlayedList:(t_playlist*)playlist{
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    playlist->nb_entries=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT name,fullpath,rating,play_count,length,channels,songs FROM user_stats WHERE play_count>0 ORDER BY play_count DESC,name");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                playlist->entries[playlist->nb_entries].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                playlist->entries[playlist->nb_entries].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)];
                
                //adjust label
                if ([playlist->entries[playlist->nb_entries].fullpath rangeOfString:@"?"].location!=NSNotFound) {
                    playlist->entries[playlist->nb_entries].label=[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getFullCleanFilePath:[playlist->entries[playlist->nb_entries].fullpath lastPathComponent]] ,playlist->entries[playlist->nb_entries].label];
                } else {
                    
                    if ([playlist->entries[playlist->nb_entries].fullpath rangeOfString:@"@"].location!=NSNotFound) {
                        playlist->entries[playlist->nb_entries].label=[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getFullCleanFilePath:[playlist->entries[playlist->nb_entries].fullpath lastPathComponent]],playlist->entries[playlist->nb_entries].label];
                    }
                }
                
                playlist->entries[playlist->nb_entries].ratings=(signed char)sqlite3_column_int(stmt,2);
                
                playlist->entries[playlist->nb_entries].playcounts=(short int)sqlite3_column_int(stmt,3);
                playlist->entries[playlist->nb_entries].song_length=(int)sqlite3_column_int(stmt,4);
                playlist->entries[playlist->nb_entries].channels_nb=(char)sqlite3_column_int(stmt,5);
                playlist->entries[playlist->nb_entries].songs=(int)sqlite3_column_int(stmt,6);
                playlist->nb_entries++;
                if (playlist->nb_entries>=MAX_PL_ENTRIES) {
                    MDZELog("max entries reached (%d)",MAX_PL_ENTRIES);
                    break;
                }
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
}

int getPlaylistStatsDBmod(t_playlist *pl) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    int ret=0;
    
    if (pl==NULL) return ret;
    if (pl->nb_entries==0) return ret;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
#define SEARCH_CHUNK_SIZE 128
        char sqlStatement[1024*32];
        bool found_entry[SEARCH_CHUNK_SIZE];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //sqlite3_exec(db, "PRAGMA synchronous=OFF", NULL, NULL, NULL);
          //      sqlite3_exec(db, "PRAGMA count_changes=OFF", NULL, NULL, NULL);
                sqlite3_exec(db, "PRAGMA journal_mode=MEMORY", NULL, NULL, NULL);
                sqlite3_exec(db, "PRAGMA temp_store=MEMORY", NULL, NULL, NULL);

        
        int i=0;

        i=0;
        while (i<pl->nb_entries) {
            //printf("i:%d\n",i);
            snprintf(sqlStatement,sizeof(sqlStatement),"SELECT fullpath,play_count,rating,length,channels,songs FROM user_stats WHERE fullpath IN (\"%s\"",[pl->entries[i].fullpath UTF8String]);
            
            int j=1;
            int k=strlen(sqlStatement);
            found_entry[0]=NO;
            while (((i+j)<pl->nb_entries)&&(j<SEARCH_CHUNK_SIZE)) {
                int l=strlen([pl->entries[i+j].fullpath UTF8String]);
                if ((k+l+1024)<sizeof(sqlStatement)) snprintf(sqlStatement+k,sizeof(sqlStatement)-k,",\"%s\"",[pl->entries[i+j].fullpath UTF8String]);
                else break;
                k+=l+3;
                found_entry[j]=NO;
                j++;
            }
            k=strlen(sqlStatement);
            sqlStatement[k]=')';
            sqlStatement[k+1]=0;
            
//                    snprintf(sqlStatement,sizeof(sqlStatement),"SELECT s.play_count,s.rating,s.avg_rating,s.length,s.channels,s.songs FROM tmp_plnow p LEFT OUTER JOIN user_stats s ON p.fullpath=s.fullpath");
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            
            int ii=i;
            int imax=i+j;
            if (err==SQLITE_OK){
                
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    ret++;
                    const char *dbstr=(const char *)sqlite3_column_text(stmt, 0);
                    for (int kk=ii;kk<imax;kk++) {
                        if (found_entry[kk-ii]==NO)
                            if (strcmp([pl->entries[kk].fullpath UTF8String],dbstr)==0) {
                                pl->entries[kk].playcounts=(short int)sqlite3_column_int(stmt, 1);
                                pl->entries[kk].ratings=(signed char)sqlite3_column_int(stmt, 2);
                                if (pl->entries[kk].ratings<0) pl->entries[kk].ratings=0;
                                if (pl->entries[kk].ratings>5) pl->entries[kk].ratings=5;
                                pl->entries[kk].song_length=(int)sqlite3_column_int(stmt, 3);
                                pl->entries[kk].channels_nb=(char)sqlite3_column_int(stmt, 4);
                                pl->entries[kk].songs=(int)sqlite3_column_int(stmt, 5);
                                found_entry[kk-ii]=YES;
                                //ii++;
                                break;
                            }
                    }
                    
                }
                sqlite3_finalize(stmt);
            } else {
                MDZELog("ErrSQL : %d",err);
                break;
            }
            i=imax;
        }
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret;
}


-(void) reloadNowPlaying {
    for (int i=0;i<detailViewController.mPlaylist_size;i++) {
        playlist->entries[i].label=[[NSString alloc] initWithString:detailViewController.mPlaylist[i].mPlaylistFilename];
        playlist->entries[i].fullpath=[[NSString alloc ] initWithString:detailViewController.mPlaylist[i].mPlaylistFilepath];
        playlist->entries[i].ratings=detailViewController.mPlaylist[i].mPlaylistRating;
    }
    
    playlist->nb_entries=detailViewController.mPlaylist_size;
    playlist->playlist_name=nil;
    playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Now playing",@"")];
    playlist->playlist_id=nil;
    
    getPlaylistStatsDBmod(playlist);
    
}

-(void) refreshViewAfterDownload {
}

-(void) traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    [self.tableView reloadData];
    
    if (show_playlist&&(currentPlayedEntry>=0)&&(integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING)&&(playlist->nb_entries)) {
        NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
        int pos=currentPlayedEntry+1;
        if ((mDetailPlayerMode==0) && (integrated_playlist==0)) pos++;
        if (pos<[self.tableView numberOfRowsInSection:0]) [self.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:pos] animated:NO scrollPosition:UITableViewScrollPositionMiddle];
    }
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
    
    if (keys) {
        //[keys release];
        keys=nil;
    }
    if (list) {
        //[list release];
        list=nil;
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
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {
            local_entries_data[i].rating=-1;
        }
    }
    if (search_local_nb_entries) {
        for (int i=0;i<search_local_nb_entries;i++) {
            search_local_entries_data[i].rating=-1;
        }
    }
    /////////////
    
    if (show_playlist) self.navigationItem.title=[NSString stringWithFormat:@"%@",playlist->playlist_name];
    else self.navigationItem.title=NSLocalizedString(@"Browser_Playlists_MainKey",@"");
    
    [self fillKeys];
    [self.tableView reloadData];
    
    /*if (currentPlayedEntry>=0) {
     NSIndexPath *myindex=[[[NSIndexPath alloc] initWithIndex:0] autorelease];
     [self.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:currentPlayedEntry] animated:NO scrollPosition:UITableViewScrollPositionMiddle];
     }*/
    
    if (detailViewController) {
        if (detailViewController.mPlaylist_size) {
            currentPlayedEntry=detailViewController.mPlaylist_pos;
        }
    }
    
    if (show_playlist&&(currentPlayedEntry>=0)&&(integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING)&&(playlist->nb_entries)) {
        NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
        int pos=currentPlayedEntry+1;
        if ((mDetailPlayerMode==0) && (integrated_playlist==0)) pos++;
        if (pos<[self.tableView numberOfRowsInSection:0]) [self.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:pos] animated:NO scrollPosition:UITableViewScrollPositionMiddle];
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

- (void)checkCreate:(NSString *)filePath {
    NSString *completePath=[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getAppHomeDirectory],filePath];
    NSError *err;
    [mFileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:&err];
}

- (void)viewDidAppear:(BOOL)animated {
    [self hideWaiting];
    
    [super viewDidAppear:animated];
    if (show_playlist&&(currentPlayedEntry>=0)&&(integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING)) {
        NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
        int pos=currentPlayedEntry+1;
        if ((mDetailPlayerMode==0) && (integrated_playlist==0)) pos++;
        if (pos<[self.tableView numberOfRowsInSection:0]) [self.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:pos] animated:YES scrollPosition:UITableViewScrollPositionMiddle];
    }
    
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
    [self.tableView reloadData];
}

// Ensure that the view controller supports rotation and that the split view can therefore show in both portrait and landscape.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    [self.tableView reloadData];
    return YES;
}

-(int) isLocalEntryInPlaylist:(NSString*)filepath {
    int nb_occur=0;
    for (int i=0;i<playlist->nb_entries;i++)
        if ([filepath compare:playlist->entries[i].fullpath]== NSOrderedSame ) nb_occur++;
    
    return nb_occur;
}

#pragma mark -
#pragma mark Table view data source

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return nil;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    local_flag=0;
    if (browse_depth==0) return [keys count];
    if (show_playlist) return 1;
    if ((show_playlist==0)&&(browse_depth>=2)) return 2;
    return 1;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (browse_depth==0) {
        NSDictionary *dictionary = [keys objectAtIndex:section];
        NSArray *array = [dictionary objectForKey:@"entries"];
        return [array count];
    }
    if (show_playlist) {
        if (playlist->playlist_id==nil) return (playlist->nb_entries+1);
        else return (playlist->nb_entries+2);
    }
    if (section==0) return 1;
    return (search_local?search_local_entries_count:local_entries_count);
}
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    return nil;
}

- (NSInteger)tableView:(UITableView *)tabView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    return -1;
}

#if 0
- (void)slideTableViewCell:(SESlideTableViewCell*)cell didTriggerRightButton:(NSInteger)buttonIndex {
    if ([cell.reuseIdentifier compare:@"CellA"]==NSOrderedSame) {
        //DELETE action requested
        // Delete button was pressed
        NSIndexPath *indexPath = [self.tableView indexPathForCell:cell];
        bool confirmed=false;
        
        if (browse_depth==0) {
            if (indexPath.row>=5) {
                //////////////////////////////////////////////////////////////////////////////////////:
                //main playlist screen, delete a playlist
                //////////////////////////////////////////////////////////////////////////////////////:
                UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Warning",@"")
                                                                               message:NSLocalizedString(@"Are you sure you want to delete this playlist ?",@"")
                                                                        preferredStyle:UIAlertControllerStyleAlert];
                
                UIAlertAction* deleteAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Delete",@"") style:UIAlertActionStyleDestructive
                                                                     handler:^(UIAlertAction * action) {
                    if ([self deletePlaylistDB:[list objectAtIndex:indexPath.row-5]]) {
                        keys=nil;
                        list=nil;
                        [self fillKeys];
                        [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
                    }
                }];
                UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
                                                                     handler:^(UIAlertAction * action) {
                    [cell setSlideState:SESlideTableViewCellSlideStateCenter animated:YES];
                }];
                
                [alert addAction:cancelAction];
                [alert addAction:deleteAction];
                [self presentViewController:alert animated:YES completion:nil];
            }
        } else if (browse_depth==1) {
            if (show_playlist) {
                if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) {
                    //////////////////////////////////////////////////////////////////////////////////////:
                    //nowplaying playlist, remove an entry
                    //////////////////////////////////////////////////////////////////////////////////////:
                    detailViewController.mPlaylist[indexPath.row-1].mPlaylistFilename=nil;
                    detailViewController.mPlaylist[indexPath.row-1].mPlaylistFilepath=nil;
                    for (int i=indexPath.row-1;i<playlist->nb_entries-1;i++) {
                        detailViewController.mPlaylist[i].mPlaylistFilename=detailViewController.mPlaylist[i+1].mPlaylistFilename;
                        detailViewController.mPlaylist[i].mPlaylistFilepath=detailViewController.mPlaylist[i+1].mPlaylistFilepath;
                        detailViewController.mPlaylist[i].mPlaylistRating=detailViewController.mPlaylist[i+1].mPlaylistRating;
                        detailViewController.mPlaylist[i].mPlaylistCount=detailViewController.mPlaylist[i+1].mPlaylistCount;
                        detailViewController.mPlaylist[i].cover_flag=detailViewController.mPlaylist[i+1].cover_flag;
                    }
                    detailViewController.mPlaylist_size--;
                    if (detailViewController.mPlaylist_pos>=detailViewController.mPlaylist_size) detailViewController.mPlaylist_pos--;
                    if ((indexPath.row-1)<=detailViewController.mPlaylist_pos) detailViewController.mPlaylist_pos--;
                    detailViewController.mShouldUpdateInfos=1;
                                        
                    [self reloadNowPlaying];
                    
                    forceReloadCells=true;
                    [self.tableView reloadData];
                    //[self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
                    
                } else if (integrated_playlist==INTEGRATED_PLAYLIST_RANDOM) {
                    //to check
                } else if (integrated_playlist==INTEGRATED_PLAYLIST_MOSTPLAYED) {
                    short int playcount;
                    signed char rating,avg_rating;
                    
                    DBHelper::getFileStatsDBmod(playlist->entries[indexPath.row-1].fullpath,
                                                &playcount,&rating,&avg_rating);
                    playcount=0;
                    DBHelper::updateFileStatsDBmod(playlist->entries[indexPath.row-1].label,
                                                   playlist->entries[indexPath.row-1].fullpath,
                                                   playcount,rating,avg_rating);
                    
                    [self loadMostPlayedList:playlist];
                    
                    forceReloadCells=true;
                    [self.tableView reloadData];
                    //[self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
                    
                } else if (integrated_playlist==INTEGRATED_PLAYLIST_FAVORITES) {
                    short int playcount;
                    signed char rating,avg_rating;
                    DBHelper::getFileStatsDBmod(playlist->entries[indexPath.row-1].fullpath,
                                                &playcount,&rating,&avg_rating);
                    rating=0;
                    DBHelper::updateFileStatsDBmod(playlist->entries[indexPath.row-1].label,
                                                   playlist->entries[indexPath.row-1].fullpath,
                                                   playcount,rating,avg_rating);
                    
                    [self loadFavoritesList:playlist];
                    forceReloadCells=true;
                    [self.tableView reloadData];
                    //[self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
                } else if (integrated_playlist==0) {
                    if (indexPath.row>=2) {
                        //////////////////////////////////////////////////////////////////////////////////////:
                        //user playlist, remove an entry
                        //////////////////////////////////////////////////////////////////////////////////////:
                        playlist->entries[indexPath.row-2].label=nil;
                        playlist->entries[indexPath.row-2].fullpath=nil;
                        for (int i=indexPath.row-2;i<playlist->nb_entries-1;i++) {
                            playlist->entries[i].label=playlist->entries[i+1].label;
                            playlist->entries[i].fullpath=playlist->entries[i+1].fullpath;
                            playlist->entries[i].ratings=playlist->entries[i+1].ratings;
                            playlist->entries[i].playcounts=playlist->entries[i+1].playcounts;
                        }
                        playlist->nb_entries--;
                        [self replacePlaylistDBwithCurrent];
                        
                        forceReloadCells=true;
                        [self.tableView reloadData];
                        //[self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
                        
                    }
                }
            }
        }
    }
}
#endif

- (UISwipeActionsConfiguration *)tableView:(UITableView *)tableView
trailingSwipeActionsConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (browse_depth==0) {
        if (indexPath.row>=5) {
            //////////////////////////////////////////////////////////////////////////////////////:
            //main playlist screen, delete a playlist
            //////////////////////////////////////////////////////////////////////////////////////:
            UIContextualAction *deleteAction =
            [UIContextualAction contextualActionWithStyle:UIContextualActionStyleDestructive
                                                    title:NSLocalizedString(@"Delete", @"")
                                                  handler:^(UIContextualAction *action,
                                                            UIView *sourceView,
                                                            void (^completionHandler)(BOOL)) {
                
                
                UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Warning",@"")
                                                                               message:NSLocalizedString(@"Are you sure you want to delete this playlist ?",@"")
                                                                        preferredStyle:UIAlertControllerStyleAlert];
                
                UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
                    
                }];
                [alert addAction:cancelAction];
                
                UIAlertAction *saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Delete",@"") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                    if ([self deletePlaylistDB:[list objectAtIndex:indexPath.row-5]]) {
                        keys=nil;
                        list=nil;
                        [self fillKeys];
                        [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
                    }
                    [tableView reloadData];
                    completionHandler(YES);
                }];
                [alert addAction:saveAction];
                
                [self presentViewController:alert animated:YES completion:nil];
                completionHandler(YES);
            }];
        deleteAction.backgroundColor = [UIColor redColor];
        
        // Return multiple actions - they appear from right to left
        return [UISwipeActionsConfiguration configurationWithActions:@[deleteAction]];
        }
    } else if (browse_depth==1) {
        if (show_playlist) {
            if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) {
                //////////////////////////////////////////////////////////////////////////////////////:
                //nowplaying playlist, remove an entry
                //////////////////////////////////////////////////////////////////////////////////////:
                
                
                UIContextualAction *deleteAction =
                [UIContextualAction contextualActionWithStyle:UIContextualActionStyleDestructive
                                                        title:NSLocalizedString(@"Delete", @"")
                                                      handler:^(UIContextualAction *action,
                                                                UIView *sourceView,
                                                                void (^completionHandler)(BOOL)) {
                    
                    detailViewController.mPlaylist[indexPath.row-1].mPlaylistFilename=nil;
                    detailViewController.mPlaylist[indexPath.row-1].mPlaylistFilepath=nil;
                    for (int i=indexPath.row-1;i<playlist->nb_entries-1;i++) {
                        detailViewController.mPlaylist[i].mPlaylistFilename=detailViewController.mPlaylist[i+1].mPlaylistFilename;
                        detailViewController.mPlaylist[i].mPlaylistFilepath=detailViewController.mPlaylist[i+1].mPlaylistFilepath;
                        detailViewController.mPlaylist[i].mPlaylistRating=detailViewController.mPlaylist[i+1].mPlaylistRating;
                        detailViewController.mPlaylist[i].mPlaylistCount=detailViewController.mPlaylist[i+1].mPlaylistCount;
                        detailViewController.mPlaylist[i].cover_flag=detailViewController.mPlaylist[i+1].cover_flag;
                    }
                    detailViewController.mPlaylist_size--;
                    if (detailViewController.mPlaylist_pos>=detailViewController.mPlaylist_size) detailViewController.mPlaylist_pos--;
                    if ((indexPath.row-1)<=detailViewController.mPlaylist_pos) detailViewController.mPlaylist_pos--;
                    detailViewController.mShouldUpdateInfos=1;
                    
                    [self reloadNowPlaying];
                    
                    forceReloadCells=true;
                    [self.tableView reloadData];
                    completionHandler(YES);
                }];
                
                deleteAction.backgroundColor = [UIColor redColor];
                
                // Return multiple actions - they appear from right to left
                return [UISwipeActionsConfiguration configurationWithActions:@[deleteAction]];
                
            } else if (integrated_playlist==INTEGRATED_PLAYLIST_RANDOM) {
                //to check
            } else if (integrated_playlist==INTEGRATED_PLAYLIST_MOSTPLAYED) {
                
                UIContextualAction *deleteAction =
                [UIContextualAction contextualActionWithStyle:UIContextualActionStyleDestructive
                                                        title:NSLocalizedString(@"Delete", @"")
                                                      handler:^(UIContextualAction *action,
                                                                UIView *sourceView,
                                                                void (^completionHandler)(BOOL)) {
                    
                    short int playcount;
                    signed char rating,avg_rating;
                    
                    DBHelper::getFileStatsDBmod(playlist->entries[indexPath.row-1].fullpath,
                                                &playcount,&rating,&avg_rating);
                    playcount=0;
                    DBHelper::updateFileStatsDBmod(playlist->entries[indexPath.row-1].label,
                                                   playlist->entries[indexPath.row-1].fullpath,
                                                   playcount,rating,avg_rating);
                    
                    [self loadMostPlayedList:playlist];
                    
                    forceReloadCells=true;
                    [self.tableView reloadData];
                    completionHandler(YES);
                }];
                
                deleteAction.backgroundColor = [UIColor redColor];
                
                // Return multiple actions - they appear from right to left
                return [UISwipeActionsConfiguration configurationWithActions:@[deleteAction]];
                
            } else if (integrated_playlist==INTEGRATED_PLAYLIST_FAVORITES) {
                UIContextualAction *deleteAction =
                [UIContextualAction contextualActionWithStyle:UIContextualActionStyleDestructive
                                                        title:NSLocalizedString(@"Delete", @"")
                                                      handler:^(UIContextualAction *action,
                                                                UIView *sourceView,
                                                                void (^completionHandler)(BOOL)) {
                    short int playcount;
                    signed char rating,avg_rating;
                    DBHelper::getFileStatsDBmod(playlist->entries[indexPath.row-1].fullpath,
                                                &playcount,&rating,&avg_rating);
                    rating=0;
                    DBHelper::updateFileStatsDBmod(playlist->entries[indexPath.row-1].label,
                                                   playlist->entries[indexPath.row-1].fullpath,
                                                   playcount,rating,avg_rating);
                    
                    [self loadFavoritesList:playlist];
                    forceReloadCells=true;
                    [self.tableView reloadData];
                    completionHandler(YES);
                }];
                
                deleteAction.backgroundColor = [UIColor redColor];
                
                // Return multiple actions - they appear from right to left
                return [UISwipeActionsConfiguration configurationWithActions:@[deleteAction]];
            } else if (integrated_playlist==0) {
                if (indexPath.row>=2) {
                    //////////////////////////////////////////////////////////////////////////////////////:
                    //user playlist, remove an entry
                    //////////////////////////////////////////////////////////////////////////////////////:
                    
                    UIContextualAction *deleteAction =
                    [UIContextualAction contextualActionWithStyle:UIContextualActionStyleDestructive
                                                            title:NSLocalizedString(@"Delete", @"")
                                                          handler:^(UIContextualAction *action,
                                                                    UIView *sourceView,
                                                                    void (^completionHandler)(BOOL)) {
                        playlist->entries[indexPath.row-2].label=nil;
                        playlist->entries[indexPath.row-2].fullpath=nil;
                        for (int i=indexPath.row-2;i<playlist->nb_entries-1;i++) {
                            playlist->entries[i].label=playlist->entries[i+1].label;
                            playlist->entries[i].fullpath=playlist->entries[i+1].fullpath;
                            playlist->entries[i].ratings=playlist->entries[i+1].ratings;
                            playlist->entries[i].playcounts=playlist->entries[i+1].playcounts;
                        }
                        playlist->nb_entries--;
                        [self replacePlaylistDBwithCurrent];
                        
                        forceReloadCells=true;
                        [self.tableView reloadData];
                        completionHandler(YES);
                    }];
                    
                    deleteAction.backgroundColor = [UIColor redColor];
                    
                    // Return multiple actions - they appear from right to left
                    return [UISwipeActionsConfiguration configurationWithActions:@[deleteAction]];
                }
            }
        }
    }
    return nil;
}


- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"Cell";
    NSString *cellValue;
    const NSInteger TOP_LABEL_TAG = 1001;
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    const NSInteger BOTTOM_IMAGE_TAG = 1003;
    const NSInteger ACT_IMAGE_TAG = 1004;
    const NSInteger SECACT_IMAGE_TAG = 1005;
    //UILabel *topLabel;
    CBAutoScrollLabel *topLabel;
    UILabel *bottomLabel;
    UIImageView *bottomImageView;
    UIButton *actionView,*secActionView;
    t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
    BOOL isEditing=[tabView isEditing];
    
    UITableViewCell *cell;
    
    if (forceReloadCells) {
        while ([tableView dequeueReusableCellWithIdentifier:CellIdentifier]) {}
        forceReloadCells=false;
    }
    
    cell = (UITableViewCell *)[tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
        cell.frame=CGRectMake(0,0,tabView.frame.size.width,40);
        
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
        
        //
        // Create the label for the top row of text
        //
        //topLabel = [[UILabel alloc] init];
        topLabel = [[CBAutoScrollLabel alloc] init];
        topLabel.labelSpacing = 35; // distance between start and end labels
        topLabel.pauseInterval = 3.7; // seconds of pause before scrolling starts again
        topLabel.scrollSpeed = 30; // pixels per second
        topLabel.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
        
        [cell.contentView addSubview:topLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        topLabel.tag = TOP_LABEL_TAG;
        topLabel.backgroundColor = [UIColor clearColor];
        topLabel.font = [UIFont systemFontOfSize:17 weight:MDZ_UIFONT_WEIGHT];
        //topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
         //                       ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;;
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
        //        cell.selectionStyle=UITableViewCellSelectionStyleGray;
    } else {
        //topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        topLabel = (CBAutoScrollLabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
        bottomImageView = (UIImageView *)[cell viewWithTag:BOTTOM_IMAGE_TAG];
        actionView = (UIButton *)[cell viewWithTag:ACT_IMAGE_TAG];
        secActionView = (UIButton *)[cell viewWithTag:SECACT_IMAGE_TAG];
        
//        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
//                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
        
        bottomLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
    }
    float margin=MDZ_TABVIEW_SEPARATOR_MARGIN;
    cell.layoutMargins = UIEdgeInsetsMake(0, margin, 0, margin);
    cell.separatorInset = UIEdgeInsetsMake(0, margin, 0, margin);
    
    actionView.hidden=TRUE;
    secActionView.hidden=TRUE;
    
    if (darkMode) {
        topLabel.textColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        //topLabel.highlightedTextColor = [UIColor colorWithRed:1 green:1 blue:1 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.6 green:0.6 blue:0.6 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
    } else {
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        //topLabel.highlightedTextColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0];
    }
    
    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                               0,
                               tabView.bounds.size.width -1.0 * cell.indentationWidth-32-(isEditing?32:0),
                               22);
    bottomLabel.frame = CGRectMake(1.0 * cell.indentationWidth,
                                   22,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-(isEditing?32:0),
                                   18);
    
    bottomLabel.text=@""; //default value
    bottomImageView.image=nil;
    cell.hidden=FALSE;
    cell.frame=CGRectMake(0,0,tabView.frame.size.width,40);
    cell.accessoryType = UITableViewCellAccessoryNone;
    
    // Set up the cell...
    if (browse_depth==0) {
        NSDictionary *dictionary = [keys objectAtIndex:indexPath.section];
        NSArray *array = [dictionary objectForKey:@"entries"];
        NSArray *array_details = [dictionary objectForKey:@"entries_details"];
        cellValue = [array objectAtIndex:indexPath.row];
        
        if (indexPath.row==0) { //Add playlist
            if (darkMode) topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED_DARKMODE green:ACTION_COLOR_GREEN_DARKMODE blue:ACTION_COLOR_BLUE_DARKMODE alpha:1.0];
            else topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            bottomLabel.text=[array_details objectAtIndex:indexPath.row];
            
        } else if (indexPath.row<=1) {
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            bottomLabel.text=[array_details objectAtIndex:indexPath.row];
            
        } else {
            actionView.enabled=YES;
            actionView.hidden=NO;
            actionView.frame = CGRectMake(tabView.bounds.size.width-2-32-34-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,0,34,34);
            [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateNormal];
            [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateHighlighted];
            
            [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
            [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
            
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            /*topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
             0,
             tableView.bounds.size.width -1.0 * cell.indentationWidth- 32,
             ROW_HEIGHT);*/
            bottomLabel.text=[array_details objectAtIndex:indexPath.row];
        }
    } else {
        if (show_playlist) {
            int row=indexPath.row;
            if (playlist->playlist_id==nil) row++;
            if (row==0) {  //playlist/file browser
                cellValue=NSLocalizedString(@"Add/Remove files...",@"");
                bottomLabel.text = NSLocalizedString(@"Add or remove entries from browser.",@"");
                cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                if (darkMode) topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED_DARKMODE green:ACTION_COLOR_GREEN_DARKMODE blue:ACTION_COLOR_BLUE_DARKMODE alpha:1.0];
                else topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
            }
            else if (row==1) {  //playlist/rename
                cellValue=NSLocalizedString(@"More actions...",@"");
                bottomLabel.text = NSLocalizedString(@"",@"");
                cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                if (darkMode) topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED_DARKMODE green:ACTION_COLOR_GREEN_DARKMODE blue:ACTION_COLOR_BLUE_DARKMODE alpha:1.0];
                else topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
            } else {  //playlist entries
                cellValue=playlist->entries[row-2].label;
                cell.accessoryType = UITableViewCellAccessoryNone;
                
                char signed avg_rating=-1;
                if ((playlist->entries[row-2].ratings==-1)||(playlist->entries[row-2].playcounts==-1)) {
                    DBHelper::getFileStatsDBmod(playlist->entries[row-2].fullpath,
                                                &(playlist->entries[row-2].playcounts),
                                                &(playlist->entries[row-2].ratings),
                                                &avg_rating,
                                                &(playlist->entries[row-2].song_length),
                                                &(playlist->entries[row-2].channels_nb),
                                                &(playlist->entries[row-2].songs));
                    if (playlist->playlist_id==nil) {//current queue
                        detailViewController.mPlaylist[row-2].mPlaylistRating=playlist->entries[row-2].ratings;
                    }
                }
                
                if ((playlist->entries[row-2].ratings==0)&&(avg_rating>0)) playlist->entries[row-2].ratings=1;
                
                if (playlist->entries[row-2].ratings>0) {
                    bottomImageView.image=[UIImage imageNamed:ratingImg[RATING_IMG(playlist->entries[row-2].ratings)]];
                }
                NSArray *filename_parts=[playlist->entries[row-2].fullpath componentsSeparatedByString:@"/"];
                
                NSString *tmp_str;
                
                /*if ([filename_parts count]>=3) {
                 if ([(NSString*)[filename_parts objectAtIndex:[filename_parts count]-3] compare:@"Documents"]!=NSOrderedSame) {
                 tmp_str = [NSString stringWithFormat:@"%@/%@|",[filename_parts objectAtIndex:[filename_parts count]-3],[filename_parts objectAtIndex:[filename_parts count]-2]];
                 } else tmp_str = [NSString stringWithFormat:@"%@|",[filename_parts objectAtIndex:[filename_parts count]-2]];
                 } else if ([filename_parts count]>=2) {
                 if ([(NSString*)[filename_parts objectAtIndex:[filename_parts count]-2] compare:@"Documents"]!=NSOrderedSame) {
                 tmp_str = [NSString stringWithFormat:@"%@|",[filename_parts objectAtIndex:[filename_parts count]-2]];
                 } else tmp_str = @"";
                 }*/
                tmp_str = [[filename_parts subarrayWithRange:NSMakeRange(1,[filename_parts count]-1)] componentsJoinedByString:@"/"];
                
                bottomLabel.text=[tmp_str stringByAppendingFormat:@" | Pl:%d ",playlist->entries[row-2].playcounts];
                
                
                bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+20,
                                               22,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth-32-20,
                                               18);
                
            }
        } else {
            if (indexPath.section==0) {
                cellValue=(mShowSubdir?NSLocalizedString(@"DisplayDir_MainKey",""):NSLocalizedString(@"DisplayAll_MainKey",""));
                if (darkMode) topLabel.textColor=[UIColor colorWithRed:0.4f green:0.4f blue:0.9f alpha:1.0];
                else topLabel.textColor=[UIColor colorWithRed:0.4f green:0.4f blue:0.9f alpha:1.0];
                bottomLabel.text=(mShowSubdir?NSLocalizedString(@"DisplayDir_SubKey",""):NSLocalizedString(@"DisplayAll_SubKey",""));
                
                int total_files=0;
                for (int i=0;i<(search_local?search_local_nb_entries:local_nb_entries);i++) {
                    if (cur_local_entries[i].type>0) total_files++;
                }
                
                
                NSString *strNbFiles;
                if (total_files==1) {
                    strNbFiles=[NSString stringWithFormat:NSLocalizedString(@"1 entry",@"")];
                    bottomLabel.text=[NSString stringWithFormat:@"%@ %@",(mShowSubdir?NSLocalizedString(@"DisplayDir_SubKey",""):NSLocalizedString(@"DisplayAll_SubKey","")),strNbFiles];
                } else if (total_files>1) {
                    strNbFiles=[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),total_files];
                    bottomLabel.text=[NSString stringWithFormat:@"%@ %@",(mShowSubdir?NSLocalizedString(@"DisplayDir_SubKey",""):NSLocalizedString(@"DisplayAll_SubKey","")),strNbFiles];
                } else {
                    bottomLabel.text=[NSString stringWithFormat:@"%@ /",(mShowSubdir?NSLocalizedString(@"DisplayDir_SubKey",""):NSLocalizedString(@"DisplayAll_SubKey",""))];
                    
                }
                
                
                bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                               22,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                               18);
                
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth-PRI_SEC_ACTIONS_IMAGE_SIZE-4-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                           22);
                
                [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateNormal];
                [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateHighlighted];
                [secActionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[secActionView.description componentsSeparatedByString:@";"] firstObject]];
                
                [actionView setImage:[UIImage imageNamed:@"playlist_del_all.png"] forState:UIControlStateNormal];
                [actionView setImage:[UIImage imageNamed:@"playlist_del_all.png"] forState:UIControlStateHighlighted];
                [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
                
                actionView.frame = CGRectMake(tabView.bounds.size.width-2-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                actionView.enabled=YES;
                actionView.hidden=NO;
                secActionView.frame = CGRectMake(tabView.bounds.size.width-2-PRI_SEC_ACTIONS_IMAGE_SIZE-PRI_SEC_ACTIONS_IMAGE_SIZE-4-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                secActionView.enabled=YES;
                secActionView.hidden=NO;
                
            } else {
                
                if (self.tableView.refreshControl.isRefreshing==false) {
                    if ((cur_local_entries[indexPath.row].type&16)&&((cur_local_entries[indexPath.row].type&15)==2)) { //need to confirm if true archive
                        if ([ModizFileHelper isABrowsableArchive:[ModizFileHelper getFullPathForFilePath:cur_local_entries[indexPath.row].fullpath]]) cur_local_entries[indexPath.row].type=2;
                        else cur_local_entries[indexPath.row].type=1;
                    }
                    
                }
                
                if (cur_local_entries[indexPath.row].type==0) { //directory
                    cellValue=cur_local_entries[indexPath.row].label;
                    if (darkMode) topLabel.textColor=[UIColor colorWithRed:MDZ_FOLDER_DARK_R green:MDZ_FOLDER_DARK_G blue:MDZ_FOLDER_DARK_B alpha:1.0f];
                    else topLabel.textColor=[UIColor colorWithRed:MDZ_FOLDER_LIGHT_R green:MDZ_FOLDER_LIGHT_G blue:MDZ_FOLDER_LIGHT_B alpha:1.0f];
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                               0,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                                               34);
                    
                } else  { //file
                    int nb_occur;
                    NSString *tmp_str;
                    cellValue=cur_local_entries[indexPath.row].label;
                    cell.accessoryType = UITableViewCellAccessoryNone;
                    
                    if (is_rsn&& (cur_local_entries[indexPath.row].altlabel==nil)) {
                        NSString *tmpFile=[NSString stringWithFormat:@"%@/tmpArchiveBrowser/%@",NSTemporaryDirectory(),cur_local_entries[indexPath.row].label];
                        SPCTag tag;
                        if ([SPCTagParser parseTagsFromFile:tmpFile tag:&tag]) {
                            cur_local_entries[indexPath.row].altlabel=[NSString stringWithFormat:@"%.3d-%s",indexPath.row,tag.songName];
                            [SPCTagParser freeTag:&tag]; // Libérer la mémoire
                        }
                    }
                    if (cur_local_entries[indexPath.row].altlabel) cellValue=cur_local_entries[indexPath.row].altlabel;
                    
                    int actionicon_offsetx=tabView.safeAreaInsets.left+tabView.safeAreaInsets.right;
                    //archive file ?
                    if ((cur_local_entries[indexPath.row].type==2)||(cur_local_entries[indexPath.row].type==3)) {
                        actionicon_offsetx=PRI_SEC_ACTIONS_IMAGE_SIZE+tabView.safeAreaInsets.left+tabView.safeAreaInsets.right;
                        //                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                        secActionView.frame = CGRectMake(tabView.bounds.size.width-2-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                        [secActionView setImage:[UIImage imageNamed:@"arc_details.png"] forState:UIControlStateNormal];
                        [secActionView setImage:[UIImage imageNamed:@"arc_details.png"] forState:UIControlStateHighlighted];
                        [secActionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                        [secActionView addTarget: self action: @selector(accessoryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                        [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[secActionView.description componentsSeparatedByString:@";"] firstObject]];
                        secActionView.enabled=YES;
                        secActionView.hidden=NO;
                    }
                    
                    
                    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                               0,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,
                                               22);
                    
                    
                    if (cur_local_entries[indexPath.row].rating==-1) {
                        char signed avg_rating;
                        DBHelper::getFileStatsDBmod(cur_local_entries[indexPath.row].fullpath,
                                                    &cur_local_entries[indexPath.row].playcount,
                                                    &cur_local_entries[indexPath.row].rating,&avg_rating);
                        if ((cur_local_entries[indexPath.row].rating==0)&&(avg_rating>0))
                            cur_local_entries[indexPath.row].rating=1;
                    }
                    if (cur_local_entries[indexPath.row].rating>0) {
                        bottomImageView.image=[UIImage imageNamed:ratingImg[RATING_IMG(cur_local_entries[indexPath.row].rating)]];
                    }
                    tmp_str = [NSString stringWithFormat:@"Pl:%d",cur_local_entries[indexPath.row].playcount];
                    
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+20,
                                                   22,
                                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-PRI_SEC_ACTIONS_IMAGE_SIZE-20-actionicon_offsetx,
                                                   18);
                    if ((nb_occur=[self isLocalEntryInPlaylist:cur_local_entries[indexPath.row].fullpath])) {
                        
                        [actionView setImage:[UIImage imageNamed:@"playlist_del.png"] forState:UIControlStateNormal];
                        [actionView setImage:[UIImage imageNamed:@"playlist_del.png"] forState:UIControlStateHighlighted];
                        [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                        [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                        [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
                        actionView.frame = CGRectMake(tabView.bounds.size.width-2-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                        actionView.enabled=YES;
                        actionView.hidden=NO;
                        
                        if (nb_occur==1) bottomLabel.text=[NSString stringWithFormat:@"Added 1 time. %@",tmp_str];
                        else bottomLabel.text=[NSString stringWithFormat:@"Added %d times. %@",nb_occur,tmp_str];
                        if (darkMode) topLabel.textColor=[UIColor colorWithRed:1-0.4f green:1-0.4f blue:1-0.4f alpha:1.0f];
                        else topLabel.textColor=[UIColor colorWithRed:0.4f green:0.4f blue:0.4f alpha:1.0f];
                    } else {
                        bottomLabel.text=[NSString stringWithFormat:@"Not in playlist. %@",tmp_str];
                    }
                }
            }
        }
    }
    topLabel.text = cellValue;
    
    return cell;
}

// Override to support editing the table view.
- (void)tableView:(UITableView *)tabView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    int rowofs=(integrated_playlist?1:2);
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        if (show_playlist&&(indexPath.row>=rowofs)) { //delete playlist entry
            if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) { //current queue
                detailViewController.mPlaylist[indexPath.row-rowofs].mPlaylistFilename=nil;
                detailViewController.mPlaylist[indexPath.row-rowofs].mPlaylistFilepath=nil;
                for (int i=indexPath.row-rowofs;i<playlist->nb_entries-1;i++) {
                    detailViewController.mPlaylist[i].mPlaylistFilename=detailViewController.mPlaylist[i+1].mPlaylistFilename;
                    detailViewController.mPlaylist[i].mPlaylistFilepath=detailViewController.mPlaylist[i+1].mPlaylistFilepath;
                    detailViewController.mPlaylist[i].mPlaylistRating=detailViewController.mPlaylist[i+1].mPlaylistRating;
                    detailViewController.mPlaylist[i].mPlaylistCount=detailViewController.mPlaylist[i+1].mPlaylistCount;
                    detailViewController.mPlaylist[i].cover_flag=detailViewController.mPlaylist[i+1].cover_flag;
                }
                detailViewController.mPlaylist_size--;
                if (detailViewController.mPlaylist_pos>=detailViewController.mPlaylist_size) detailViewController.mPlaylist_pos--;
                if ((indexPath.row-1)<=detailViewController.mPlaylist_pos) detailViewController.mPlaylist_pos--;
                detailViewController.mShouldUpdateInfos=1;
                
                [self reloadNowPlaying];
                
                [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
            } else if (integrated_playlist==INTEGRATED_PLAYLIST_RANDOM) { //random, TODO
                
                //[self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
            } else if (integrated_playlist==INTEGRATED_PLAYLIST_MOSTPLAYED) { //most played: reset playcount
                short int playcount;
                signed char rating,avg_rating;
                DBHelper::getFileStatsDBmod(playlist->entries[indexPath.row-rowofs].fullpath,
                                            &playcount,&rating,&avg_rating);
                playcount=0;
                DBHelper::updateFileStatsDBmod(playlist->entries[indexPath.row-rowofs].label,
                                               playlist->entries[indexPath.row-rowofs].fullpath,
                                               playcount,rating,avg_rating);
                [self loadMostPlayedList:playlist];
                [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
            } else if (integrated_playlist==INTEGRATED_PLAYLIST_FAVORITES) {  //favorites: reset rating
                short int playcount;
                signed char rating,avg_rating;
                DBHelper::getFileStatsDBmod(playlist->entries[indexPath.row-rowofs].fullpath,
                                            &playcount,&rating,&avg_rating);
                rating=0;
                DBHelper::updateFileStatsDBmod(playlist->entries[indexPath.row-rowofs].label,
                                               playlist->entries[indexPath.row-rowofs].fullpath,
                                               playcount,rating,avg_rating);
                [self loadFavoritesList:playlist];
                [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
            } else {
                playlist->entries[indexPath.row-rowofs].label=nil;
                playlist->entries[indexPath.row-rowofs].fullpath=nil;
                for (int i=indexPath.row-rowofs;i<playlist->nb_entries-1;i++) {
                    playlist->entries[i].label=playlist->entries[i+1].label;
                    playlist->entries[i].fullpath=playlist->entries[i+1].fullpath;
                    playlist->entries[i].ratings=playlist->entries[i+1].ratings;
                    playlist->entries[i].playcounts=playlist->entries[i+1].playcounts;
                }
                playlist->nb_entries--;
                [tabView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
                [self replacePlaylistDBwithCurrent];
            }
        }
        if ((browse_depth==0)&&(indexPath.row>=5)) {  //delete a playlist
            if ([self deletePlaylistDB:[list objectAtIndex:indexPath.row-5]]) {
                
                keys=nil;
                list=nil;
                [self fillKeys];
                [tabView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
                //[tabView reloadData];
                
            }
        }
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}
- (NSIndexPath *)tableView:(UITableView *)tabView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)sourceIndexPath toProposedIndexPath:(NSIndexPath *)proposedDestinationIndexPath {
    int rowofs=(integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING?1:2);
    if (show_playlist) {
        if (proposedDestinationIndexPath.row<rowofs) {
            NSIndexPath *newIndexPath=[[NSIndexPath alloc] initWithIndex:0];
            return [newIndexPath indexPathByAddingIndex:rowofs];
        }
    }
    return proposedDestinationIndexPath;
}
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tabView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
    int rowofs=(integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING?1:2);
    if (show_playlist&&(fromIndexPath.row&&toIndexPath.row>=rowofs)) {
        signed char tmpR=playlist->entries[fromIndexPath.row-rowofs].ratings;
        short int tmpC=playlist->entries[fromIndexPath.row-rowofs].playcounts;
        NSString *tmpF=playlist->entries[fromIndexPath.row-rowofs].fullpath;
        NSString *tmpL=playlist->entries[fromIndexPath.row-rowofs].label;
        if (toIndexPath.row<fromIndexPath.row) {
            for (int i=fromIndexPath.row-rowofs;i>toIndexPath.row-rowofs;i--) {
                playlist->entries[i].label=playlist->entries[i-1].label;
                playlist->entries[i].fullpath=playlist->entries[i-1].fullpath;
                playlist->entries[i].ratings=playlist->entries[i-1].ratings;
                playlist->entries[i].playcounts=playlist->entries[i-1].playcounts;
            }
            playlist->entries[toIndexPath.row-rowofs].label=tmpL;
            playlist->entries[toIndexPath.row-rowofs].fullpath=tmpF;
            playlist->entries[toIndexPath.row-rowofs].ratings=tmpR;
            playlist->entries[toIndexPath.row-rowofs].playcounts=tmpC;
        } else {
            for (int i=fromIndexPath.row-rowofs;i<toIndexPath.row-rowofs;i++) {
                playlist->entries[i].label=playlist->entries[i+1].label;
                playlist->entries[i].fullpath=playlist->entries[i+1].fullpath;
                playlist->entries[i].ratings=playlist->entries[i+1].ratings;
                playlist->entries[i].playcounts=playlist->entries[i+1].playcounts;
            }
            playlist->entries[toIndexPath.row-rowofs].label=tmpL;
            playlist->entries[toIndexPath.row-rowofs].fullpath=tmpF;
            playlist->entries[toIndexPath.row-rowofs].ratings=tmpR;
            playlist->entries[toIndexPath.row-rowofs].playcounts=tmpC;
        }
        
        if (playlist->playlist_id) [self replacePlaylistDBwithCurrent];
        else {
            t_plPlaylist_entry tmpF;
            tmpF=detailViewController.mPlaylist[fromIndexPath.row-rowofs];
            if (toIndexPath.row<fromIndexPath.row) {
                for (int i=fromIndexPath.row-rowofs;i>toIndexPath.row-rowofs;i--) {
                    detailViewController.mPlaylist[i]=detailViewController.mPlaylist[i-1];
                }
                detailViewController.mPlaylist[toIndexPath.row-rowofs]=tmpF;
            } else {
                for (int i=fromIndexPath.row-rowofs;i<toIndexPath.row-rowofs;i++) {
                    detailViewController.mPlaylist[i]=detailViewController.mPlaylist[i+1];
                }
                detailViewController.mPlaylist[toIndexPath.row-rowofs]=tmpF;
            }
            if ((fromIndexPath.row-rowofs>detailViewController.mPlaylist_pos)&&(toIndexPath.row-rowofs<=detailViewController.mPlaylist_pos)) detailViewController.mPlaylist_pos++;
            else if ((fromIndexPath.row-rowofs<detailViewController.mPlaylist_pos)&&(toIndexPath.row-rowofs>=detailViewController.mPlaylist_pos)) detailViewController.mPlaylist_pos--;
            else if (fromIndexPath.row-rowofs==detailViewController.mPlaylist_pos) detailViewController.mPlaylist_pos=toIndexPath.row-rowofs;
            
            detailViewController.mShouldUpdateInfos=1;
        }
    }
}
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tabView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    int rowofs=(integrated_playlist?1:2);
    if (show_playlist&&(indexPath.row>=rowofs)&&(integrated_playlist<=INTEGRATED_PLAYLIST_NOWPLAYING)) {
        return YES;
    }
    return NO;
}
- (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    int rowofs=(integrated_playlist?1:2);
    if (show_playlist&&(indexPath.row>=rowofs)) {
        if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) {
            if (indexPath.row-rowofs==detailViewController.mPlaylist_pos) return NO;
        }
        return YES;
    }
    if ((browse_depth==0)&&(indexPath.row>=5)) return YES;
    
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
    //[[super tableView] reloadData];
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
    mSearch=0;
    sBar.showsCancelButton = NO;
    [searchBar resignFirstResponder];
    shouldFillKeys=1;
    search_local=0;
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
                //1st try to find the player viewcontroller in the navigation stack
                NSArray *vcList=[self.navigationController viewControllers];
                if ([vcList count]>=2) {
                    for (int i=[vcList count]-1;i>0;i--) {
                        if ([[vcList objectAtIndex:i] isEqual:detailViewController]) {
                            [self.navigationController popToViewController:detailViewController animated:YES];
                            return;
                        }
                    }
                }
                //2nd push the player viewcontroller if not found above
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
        
        
        
    } else {
        [self showAlertMsg:@"Warning" message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"")];
    }
}

#pragma mark -
#pragma mark Table view delegate
- (void) primaryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    if (browse_depth==0) {
        if (indexPath.row>=2) { //start selected playlist
            [self freePlaylist];
            playlist=(t_playlist*)calloc(1,sizeof(t_playlist));
            mFreePlaylist=1;
            
            if (indexPath.row==2) {
                NSMutableArray *arrayLabels=[[NSMutableArray alloc] init];
                NSMutableArray *arrayFullpaths=[[NSMutableArray alloc] init];
                int pl_entries;
                pl_entries=[self loadLocalFilesRandomPL:arrayLabels fullpaths:arrayFullpaths];
                
                playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Random picks",@"")];
                playlist->playlist_id=nil;
                playlist->nb_entries=pl_entries;
                for (int i=0;i<[arrayLabels count];i++) {
                    playlist->entries[i].label=[arrayLabels objectAtIndex:i];
                    playlist->entries[i].fullpath=[arrayFullpaths objectAtIndex:i];
                    playlist->entries[i].ratings=-1;
                }
            } else if (indexPath.row==3) {
                [self loadMostPlayedList:playlist];
                playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Most played",@"")];
                playlist->playlist_id=nil;
            } else if (indexPath.row==4) {
                [self loadFavoritesList:playlist];
                playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Favorites",@"")];
                playlist->playlist_id=nil;
            } else [self loadPlayListsFromDB:[list objectAtIndex:(indexPath.row-5)] intoPlaylist:playlist];
            
            if (playlist->nb_entries) {
                
                [detailViewController play_listmodules:playlist start_index:0];
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                
                keys=nil;
                list=nil;
                [self fillKeys];
                [tableView reloadData];
            }
            [self freePlaylist];
            
        }
    } else {
        if (show_playlist) {
        } else { //browsing for playlist, remove selected file from playlist
            if (indexPath.section==0) {
                //remove all
                    for (int j=0;j<(search_local?search_local_entries_count:local_entries_count);j++)
                        if (cur_local_entries[j].type>0) {
                            int found=-1;
                            for (int ii=0;ii<playlist->nb_entries;ii++) {
                                if ([playlist->entries[ii].fullpath compare:cur_local_entries[j].fullpath]==NSOrderedSame) found=ii;
                            }
                            if (found>=0) {
                                playlist->entries[found].label=nil;
                                playlist->entries[found].fullpath=nil;
                                for (int ii=found;ii<playlist->nb_entries-1;ii++) {
                                    playlist->entries[ii].label=playlist->entries[ii+1].label;
                                    playlist->entries[ii].fullpath=playlist->entries[ii+1].fullpath;
                                    playlist->entries[ii].ratings=playlist->entries[ii+1].ratings;
                                    playlist->entries[ii].playcounts=playlist->entries[ii+1].playcounts;
                                }
                                playlist->nb_entries--;
                                
                                
                            }
                        }
                [self replacePlaylistDBwithCurrent];
                [tableView reloadData];
                
            } else {
                
                int found=-1;
                for (int i=0;i<playlist->nb_entries;i++) {
                    if ([playlist->entries[i].fullpath compare:cur_local_entries[indexPath.row].fullpath]==NSOrderedSame) found=i;
                }
                if (found>=0) {
                    playlist->entries[found].label=nil;
                    playlist->entries[found].fullpath=nil;
                    for (int i=found;i<playlist->nb_entries-1;i++) {
                        playlist->entries[i].label=playlist->entries[i+1].label;
                        playlist->entries[i].fullpath=playlist->entries[i+1].fullpath;
                        playlist->entries[i].ratings=playlist->entries[i+1].ratings;
                        playlist->entries[i].playcounts=playlist->entries[i+1].playcounts;
                    }
                    playlist->nb_entries--;
                    
                    [self replacePlaylistDBwithCurrent];
                    [tableView reloadData];
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
    
    t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    
    if (show_playlist) {
    } else { //browsing for playlist, add selected file to playlist
        if (indexPath.section==0) {
            //add all
                for (int j=0;j<(search_local?search_local_entries_count:local_entries_count);j++) {
                    if (cur_local_entries[j].type>0) {
                        if (playlist->nb_entries<MAX_PL_ENTRIES) {
                            playlist->nb_entries++;
                            
                            playlist->entries[playlist->nb_entries-1].label=[[NSString alloc] initWithFormat:@"%@",cur_local_entries[j].label];
                            playlist->entries[playlist->nb_entries-1].fullpath=[[NSString alloc] initWithFormat:@"%@",cur_local_entries[j].fullpath];
                            
                            playlist->entries[playlist->nb_entries-1].ratings=cur_local_entries[j].rating;
                            playlist->entries[playlist->nb_entries-1].playcounts=cur_local_entries[j].playcount;
                            //TODO : optimization is possible => to do only 1 insert into DB
                            [self addToPlaylistDB:playlist->playlist_id label:playlist->entries[playlist->nb_entries-1].label fullPath:playlist->entries[playlist->nb_entries-1].fullpath];
                        } else {
                            UIAlertController *alertC = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Warning",@"")
                                                                                            message:NSLocalizedString(@"Playlist is full. Delete some entries to add more.", @"")
                                                                                     preferredStyle:UIAlertControllerStyleAlert];
                            UIAlertAction* closeAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Ok",@"") style:UIAlertActionStyleCancel
                                                                                handler:^(UIAlertAction * action) {
                            }];
                            [alertC addAction:closeAction];
                            [self showAlert:alertC];
                            break;
                        }
                    }
                }
            [tableView reloadData];
            
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
    t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
    
    if (browse_depth==0) {
        NSDictionary *dictionary = [keys objectAtIndex:indexPath.section];
        NSArray *array = [dictionary objectForKey:@"entries"];
        cellValue = [array objectAtIndex:indexPath.row];
        
        [self freePlaylist];
        playlist=(t_playlist*)calloc(1,sizeof(t_playlist));
        mFreePlaylist=1;
        
        
        if (indexPath.row==0) { //new playlist
            [self createNewPlaylist];
        }
        if ((indexPath.row==1)&&(detailViewController.mPlaylist_size)) { //display current queue
            playlist->nb_entries=detailViewController.mPlaylist_size;
            playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Now playing",@"")];
            playlist->playlist_id=nil;
            
            
            if (childController == nil) childController = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            //set new title
            childController.title = playlist->playlist_name;
            ((RootViewControllerPlaylist*)childController)->show_playlist=1;
            
            // Set new directory
            ((RootViewControllerPlaylist*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerPlaylist*)childController)->detailViewController=detailViewController;
            ((RootViewControllerPlaylist*)childController)->playlist=playlist;
            ((RootViewControllerPlaylist*)childController)->integrated_playlist=INTEGRATED_PLAYLIST_NOWPLAYING;
            ((RootViewControllerPlaylist*)childController)->mFreePlaylist=0;
//            childController.view.frame=self.view.frame;
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
            keys=nil;
            list=nil;
            
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
            
            playlist=NULL;
        }
        if (indexPath.row==2) { //random picks
            NSMutableArray *arrayLabels=[[NSMutableArray alloc] init];
            NSMutableArray *arrayFullpaths=[[NSMutableArray alloc] init];
            int pl_entries;
            pl_entries=[self loadLocalFilesRandomPL:arrayLabels fullpaths:arrayFullpaths];
            
            playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Random picks",@"")];
            playlist->playlist_id=nil;
            playlist->nb_entries=pl_entries;
            for (int i=0;i<[arrayLabels count];i++) {
                playlist->entries[i].label=[arrayLabels objectAtIndex:i];
                playlist->entries[i].fullpath=[arrayFullpaths objectAtIndex:i];
                playlist->entries[i].ratings=-1;
            }
            
            if (childController == nil) childController = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            //set new title
            childController.title = playlist->playlist_name;
            ((RootViewControllerPlaylist*)childController)->show_playlist=1;
            
            // Set new directory
            ((RootViewControllerPlaylist*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerPlaylist*)childController)->detailViewController=detailViewController;
            ((RootViewControllerPlaylist*)childController)->playlist=playlist;
            ((RootViewControllerPlaylist*)childController)->integrated_playlist=INTEGRATED_PLAYLIST_RANDOM;
            ((RootViewControllerPlaylist*)childController)->mFreePlaylist=0;
//            childController.view.frame=self.view.frame;
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
            keys=nil;
            list=nil;
            
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
        }
        if (indexPath.row==3) { //most played
            [self loadMostPlayedList:playlist];
            playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Most played",@"")];
            playlist->playlist_id=nil;
            
            if (childController == nil) childController = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            //set new title
            childController.title = playlist->playlist_name;
            ((RootViewControllerPlaylist*)childController)->show_playlist=1;
            
            // Set new directory
            ((RootViewControllerPlaylist*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerPlaylist*)childController)->detailViewController=detailViewController;
            ((RootViewControllerPlaylist*)childController)->playlist=playlist;
            ((RootViewControllerPlaylist*)childController)->integrated_playlist=INTEGRATED_PLAYLIST_MOSTPLAYED;
            ((RootViewControllerPlaylist*)childController)->mFreePlaylist=0;
//            childController.view.frame=self.view.frame;
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
            keys=nil;
            list=nil;
            
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
        }
        if (indexPath.row==4) { //favorites
            [self loadFavoritesList:playlist];
            playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Favorites",@"")];
            playlist->playlist_id=nil;
            
            if (childController == nil) childController = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            //set new title
            childController.title = playlist->playlist_name;
            ((RootViewControllerPlaylist*)childController)->show_playlist=1;
            
            // Set new directory
            ((RootViewControllerPlaylist*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerPlaylist*)childController)->detailViewController=detailViewController;
            ((RootViewControllerPlaylist*)childController)->playlist=playlist;
            ((RootViewControllerPlaylist*)childController)->integrated_playlist=INTEGRATED_PLAYLIST_FAVORITES;
            ((RootViewControllerPlaylist*)childController)->mFreePlaylist=0;
//            childController.view.frame=self.view.frame;
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
            keys=nil;
            list=nil;
            
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
        }
        if (indexPath.row>=5) {
            if (childController == nil) childController = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            //set new title
            childController.title = cellValue;
            ((RootViewControllerPlaylist*)childController)->show_playlist=1;
            //get list id
            [self loadPlayListsFromDB:[list objectAtIndex:(indexPath.row-5)] intoPlaylist:playlist];
            
            // Set new directory
            ((RootViewControllerPlaylist*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerPlaylist*)childController)->detailViewController=detailViewController;
            ((RootViewControllerPlaylist*)childController)->playlist=playlist;
            ((RootViewControllerPlaylist*)childController)->mFreePlaylist=0;
//            childController.view.frame=self.view.frame;
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
            keys=nil;
            list=nil;
            
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
        }
        
    } else {
        if (show_playlist) {
            int row=indexPath.row;
            if (playlist->playlist_id==nil) row++;
            if (row>=2) {//start playlist and position it at selected entry
                //						self.navigationController.navigationBar.hidden = YES;
                [detailViewController play_listmodules:playlist start_index:(row-2)];
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                
                
                [tabView reloadData];
                
                [self.tableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionMiddle];
                
            } else if (row==0 ){ //add new entry to current playlist
                if (childController == nil) childController = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                else {			// Don't cache childviews
                }
                //set new title
                childController.title = playlist->playlist_name;
                // Set new directory
                newPlaylist=0;
                ((RootViewControllerPlaylist*)childController)->browse_depth = 2;
                ((RootViewControllerPlaylist*)childController)->detailViewController=detailViewController;
                ((RootViewControllerPlaylist*)childController)->playlist=playlist;
                ((RootViewControllerPlaylist*)childController)->show_playlist=0;
                ((RootViewControllerPlaylist*)childController)->mFreePlaylist=0;
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
            } else if (row==1 ){ //playlist actions
                if (playlist->playlist_id) {
                    UIAlertController *alertC;
                    
                    alertC = [UIAlertController alertControllerWithTitle:nil
                                                                 message:nil
                                                          preferredStyle:UIAlertControllerStyleActionSheet];
                    
                    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
                                                                         handler:^(UIAlertAction * action) {
                    }];
                    [alertC addAction:cancelAction];
                    
                    UIAlertAction* renameAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Rename",@"") style:UIAlertActionStyleDefault
                                                                         handler:^(UIAlertAction * action) {
                        [self renamePlaylist];
                    }];
                    [alertC addAction:renameAction];
                    
                    UIAlertAction* savetoNewAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Save as a new playlist",@"") style:UIAlertActionStyleDefault
                                                                         handler:^(UIAlertAction * action) {
                        [self saveToNewPlaylist];
                    }];
                    [alertC addAction:savetoNewAction];
                    
                    UIAlertAction* editAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Edit",@"") style:UIAlertActionStyleDefault
                                                                       handler:^(UIAlertAction * action) {
                        [self editPlaylist];
                    }];
                    [alertC addAction:editAction];
                    
                    UIAlertAction* shuffleAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Shuffle",@"") style:UIAlertActionStyleDefault
                                                                          handler:^(UIAlertAction * action) {
                        [self shufflePlaylist];
                    }];
                    [alertC addAction:shuffleAction];
                    
                    UIAlertAction* sortAZAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Sort A->Z",@"") style:UIAlertActionStyleDefault
                                                                         handler:^(UIAlertAction * action) {
                        [self sortAZPlaylist:false];
                    }];
                    [alertC addAction:sortAZAction];
                    
                    UIAlertAction* sortZAAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Sort Z->A",@"") style:UIAlertActionStyleDefault
                                                                         handler:^(UIAlertAction * action) {
                        [self sortZAPlaylist:false];
                    }];
                    [alertC addAction:sortZAAction];
                    
                    UIAlertAction* sortAZActionFP = [UIAlertAction actionWithTitle:NSLocalizedString(@"Sort A->Z (fullpath)",@"") style:UIAlertActionStyleDefault
                                                                           handler:^(UIAlertAction * action) {
                        [self sortAZPlaylist:true];
                    }];
                    [alertC addAction:sortAZActionFP];
                    
                    UIAlertAction* sortZAActionFP = [UIAlertAction actionWithTitle:NSLocalizedString(@"Sort Z->A (fullpath)",@"") style:UIAlertActionStyleDefault
                                                                           handler:^(UIAlertAction * action) {
                        [self sortZAPlaylist:true];
                    }];
                    [alertC addAction:sortZAActionFP];
                    
                    UIAlertAction* ExportFilesActionFP = [UIAlertAction actionWithTitle:NSLocalizedString(@"Export files",@"") style:UIAlertActionStyleDefault
                                                                           handler:^(UIAlertAction * action) {
                        [self exportFiles];
                    }];
                    [alertC addAction:ExportFilesActionFP];
                    
                    UIAlertAction* deleteAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Delete",@"") style:UIAlertActionStyleDestructive
                                                                         handler:^(UIAlertAction * action) {
                        [self deletePlaylist];
                    }];
                    [alertC addAction:deleteAction];
                    
                    [self showAlert:alertC];
                } else { //"now playing", "most played", "favorites" playlists -> does not exist in DB
                    UIAlertController *alertC;
                    
                    if (integrated_playlist<=INTEGRATED_PLAYLIST_NOWPLAYING) {
                        alertC = [UIAlertController alertControllerWithTitle:nil
                                                                     message:nil
                                                              preferredStyle:UIAlertControllerStyleActionSheet];
                        
                        UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
                                                                             handler:^(UIAlertAction * action) {
                        }];
                        [alertC addAction:cancelAction];
                        
                        UIAlertAction* saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Save",@"") style:UIAlertActionStyleDefault
                                                                           handler:^(UIAlertAction * action) {
                            [self saveToNewPlaylist];
                        }];
                        [alertC addAction:saveAction];
                        
                        UIAlertAction* editAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Edit",@"") style:UIAlertActionStyleDefault
                                                                           handler:^(UIAlertAction * action) {
                            [self editPlaylist];
                        }];
                        [alertC addAction:editAction];
                        
                        UIAlertAction* shuffleAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Shuffle",@"") style:UIAlertActionStyleDefault
                                                                              handler:^(UIAlertAction * action) {
                            [self shufflePlaylist];
                        }];
                        [alertC addAction:shuffleAction];
                        
                        UIAlertAction* sortAZAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Sort A->Z",@"") style:UIAlertActionStyleDefault
                                                                             handler:^(UIAlertAction * action) {
                            [self sortAZPlaylist:false];
                        }];
                        [alertC addAction:sortAZAction];
                        
                        UIAlertAction* sortZAAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Sort Z->A",@"") style:UIAlertActionStyleDefault
                                                                             handler:^(UIAlertAction * action) {
                            [self sortZAPlaylist:false];
                        }];
                        [alertC addAction:sortZAAction];
                        
                        UIAlertAction* sortAZActionFP = [UIAlertAction actionWithTitle:NSLocalizedString(@"Sort A->Z (fullpath)",@"") style:UIAlertActionStyleDefault
                                                                               handler:^(UIAlertAction * action) {
                            [self sortAZPlaylist:true];
                        }];
                        [alertC addAction:sortAZActionFP];
                        
                        UIAlertAction* sortZAActionFP = [UIAlertAction actionWithTitle:NSLocalizedString(@"Sort Z->A (fullpath)",@"") style:UIAlertActionStyleDefault
                                                                               handler:^(UIAlertAction * action) {
                            [self sortZAPlaylist:true];
                        }];
                        [alertC addAction:sortZAActionFP];
                        
                        [self showAlert:alertC];
                    } else if ((integrated_playlist==INTEGRATED_PLAYLIST_MOSTPLAYED)||(integrated_playlist==INTEGRATED_PLAYLIST_FAVORITES)) {
                        alertC = [UIAlertController alertControllerWithTitle:nil
                                                                     message:nil
                                                              preferredStyle:UIAlertControllerStyleActionSheet];
                        UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
                                                                             handler:^(UIAlertAction * action) {
                        }];
                        [alertC addAction:cancelAction];
                        
                        UIAlertAction* saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Save",@"") style:UIAlertActionStyleDefault
                                                                           handler:^(UIAlertAction * action) {
                            [self saveToNewPlaylist];
                        }];
                        [alertC addAction:saveAction];
                        
                        UIAlertAction* editAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Edit",@"") style:UIAlertActionStyleDefault
                                                                           handler:^(UIAlertAction * action) {
                            [self editPlaylist];
                        }];
                        [alertC addAction:editAction];
                        
                        [self showAlert:alertC];
                        
                    } else if (integrated_playlist==INTEGRATED_PLAYLIST_RANDOM) {
                        alertC = [UIAlertController alertControllerWithTitle:nil
                                                                     message:nil
                                                              preferredStyle:UIAlertControllerStyleActionSheet];
                        UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
                                                                             handler:^(UIAlertAction * action) {
                        }];
                        [alertC addAction:cancelAction];
                        
                        UIAlertAction* saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Save",@"") style:UIAlertActionStyleDefault
                                                                           handler:^(UIAlertAction * action) {
                            [self saveToNewPlaylist];
                        }];
                        [alertC addAction:saveAction];
                        
                        [self showAlert:alertC];
                        
                    }
                }
            }
        } else { //browsing for playlist
            if (indexPath.section==0) {
                mShowSubdir^=1;
                shouldFillKeys=1;
                
                [self showWaiting];
                [self flushMainLoop];
                
                [self fillKeys];
                [tabView reloadData];
                [self.tableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionMiddle];
                
                [self hideWaiting];
            } else {
                cellValue=cur_local_entries[indexPath.row].label;
                
                if (cur_local_entries[indexPath.row].type==0) { //Directory selected : change current directory
                    NSString *newPath=[NSString stringWithFormat:@"%@/%@",currentPath,cellValue];
                    //[newPath retain];
                    if (childController == nil) childController = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    else {// Don't cache childviews
                    }
                    //set new title
                    childController.title = cellValue;
                    // Set new depth & new directory
                    ((RootViewControllerPlaylist*)childController)->currentPath = newPath;
                    ((RootViewControllerPlaylist*)childController)->browse_depth = browse_depth+1;
                    ((RootViewControllerPlaylist*)childController)->detailViewController=detailViewController;
                    ((RootViewControllerPlaylist*)childController)->playlist=playlist;
                    ((RootViewControllerPlaylist*)childController)->mFreePlaylist=0;
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
                } else if (((cur_local_entries[indexPath.row].type==2)||(cur_local_entries[indexPath.row].type==3))&&(mAccessoryButton)) { //Archive selected or multisongs: display files inside
                    
                    [self showWaiting];
                    [self flushMainLoop];
                    
                    NSString *newPath;
                    if (mShowSubdir) newPath=[NSString stringWithString:cur_local_entries[indexPath.row].fullpath];
                    else newPath=[NSString stringWithFormat:@"%@/%@",currentPath,cellValue];
                    //[newPath retain];
                    if (childController == nil) childController = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    else {// Don't cache childviews
                    }
                    //set new title
                    childController.title = cellValue;
                    // Set new depth & new directory
                    ((RootViewControllerPlaylist*)childController)->currentPath = newPath;
                    ((RootViewControllerPlaylist*)childController)->browse_depth = browse_depth+1;
                    ((RootViewControllerPlaylist*)childController)->detailViewController=detailViewController;
                    ((RootViewControllerPlaylist*)childController)->playlist=playlist;
                    ((RootViewControllerPlaylist*)childController)->mFreePlaylist=0;
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
                    
                    
                    [self hideWaiting];
                    //				[childController autorelease];
                } else {  //File selected : add to playlist
                    if (playlist->nb_entries<MAX_PL_ENTRIES) {
                        playlist->nb_entries++;
                        playlist->entries[playlist->nb_entries-1].label=[[NSString alloc] initWithFormat:@"%@",cur_local_entries[indexPath.row].label];
                        playlist->entries[playlist->nb_entries-1].fullpath=[[NSString alloc] initWithFormat:@"%@",cur_local_entries[indexPath.row].fullpath];
                        
                        [self addToPlaylistDB:playlist->playlist_id label:playlist->entries[playlist->nb_entries-1].label fullPath:playlist->entries[playlist->nb_entries-1].fullpath];
                        [tabView reloadData];
                        
                        [self.tableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionMiddle];
                    } else {
                        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Playlist is full. Delete some entries to add more.", @"")];
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
- (void)freePlaylist {
    if (!mFreePlaylist) return;
    
    if (playlist) {
        for (int i=0;i<playlist->nb_entries;i++) {
            playlist->entries[i].label=nil;
            playlist->entries[i].fullpath=nil;
        }
        playlist->playlist_name=nil;
        playlist->playlist_id=nil;
        
        free(playlist);
        playlist=NULL;
    }
    mFreePlaylist=0;
}
- (void)dealloc {
    
    [waitingView removeFromSuperview];
    [waitingViewPlayer removeFromSuperview];
    [waitingViewExtract removeFromSuperview];
    waitingViewExtract=nil;
    waitingView=nil;
    waitingViewPlayer=nil;
    
    mSearchText=nil;
    
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {
            local_entries_data[i].label=nil;
            local_entries_data[i].fullpath=nil;
        }
            for (int j=0;j<local_entries_count;j++) {
                local_entries[j].label=nil;
                local_entries[j].fullpath=nil;
            }
            local_entries=NULL;
        free(local_entries_data);local_entries_data=NULL;
        local_nb_entries=0;
    }
    local_entries_count=0;
    
    if (search_local_nb_entries) {
            for (int j=0;j<search_local_entries_count;j++) {
                search_local_entries[j].label=nil;
                search_local_entries[j].fullpath=nil;
            }
            search_local_entries=NULL;
        search_local_nb_entries=0;
        free(search_local_entries_data);
    }
    
    mFileMngr=nil;
    if (mFreePlaylist) [self freePlaylist];
    keys=nil;
    list=nil;
    
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
        [self updateMiniPlayer];
    }
    currentPlayedEntry=detailViewController.mPlaylist_pos;
    if ((browse_depth==1)&&show_playlist&&(currentPlayedEntry>=0)&&(integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING)&&(playlist->nb_entries)) {
        NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
        
        int pos=currentPlayedEntry+1;
        if ((mDetailPlayerMode==0) && (integrated_playlist==0)) pos++;
        
        if (pos<[self.tableView numberOfRowsInSection:0]) {
            [self.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:pos] animated:YES scrollPosition:UITableViewScrollPositionMiddle];
        }
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

//- (void) cancelPushed {
//    detailViewController.mplayer.extractPendingCancel=true;
//    [detailViewController setCancelStatus:true];
//    [detailViewController hideWaitingCancel];
//    [detailViewController hideWaitingProgress];
//    [detailViewController updateWaitingDetail:NSLocalizedString(@"Cancelling...",@"")];
//        
//    [self hideWaitingCancel];
//    [self hideWaitingProgress];
//    [self updateWaitingDetail:NSLocalizedString(@"Cancelling...",@"")];
//}

- (void) cancelPushed {
    if (extractProgress) {
        [extractProgress cancel];
        [waitingViewExtract hideCancel];
        [waitingViewExtract hideProgress];
        [waitingViewExtract setDetail:NSLocalizedString(@"Cancelling...",@"")];
    } else {
        [waitingViewPlayer hideCancel];
        [waitingViewPlayer hideProgress];
        [waitingViewPlayer setDetail:NSLocalizedString(@"Cancelling...",@"")];
        
        detailViewController.mplayer.extractPendingCancel=true;
        [detailViewController setCancelStatus:true];
        [detailViewController hideWaitingCancel];
        [detailViewController hideWaitingProgress];
        [detailViewController updateWaitingDetail:NSLocalizedString(@"Cancelling...",@"")];
         
        
    }
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
        
    }  else if (context == ExtractBrowserListProgressObserverContext) {
        NSProgress *progress = object;
        
        if ([progress isCancelled]) {
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                [self.waitingViewExtract resetCancelStatus];
                self.waitingViewExtract.hidden=TRUE;
                [self.waitingViewExtract hideProgress];
                [self.tableView setUserInteractionEnabled:true];
                [self.navigationItem setHidesBackButton:NO animated:YES];
                [self.tableView reloadData];
            }];
        }
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            [self.waitingViewExtract setProgress:progress.fractionCompleted];
            if (progress.fractionCompleted>=1.0f) {
                self.waitingViewExtract.hidden=TRUE;
                [self.waitingViewExtract hideProgress];
                [self.tableView setUserInteractionEnabled:true];
                [self.navigationItem setHidesBackButton:NO animated:YES];
                [self.tableView reloadData];
            }
        }];
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}


@end
