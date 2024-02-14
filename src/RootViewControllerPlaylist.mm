//
//  RootViewController.m
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//
#define RATING_IMG(a) ( (a==5?2:(a?1:0)) )

#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define ROW_HEIGHT 40



#define LIMITED_LIST_SIZE 1024

#import "MDZUIImageView.h"

#include <sys/types.h>
#include <sys/sysctl.h>

#include "gme.h"

#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidTuneInfo.h"

#include "unzip.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;
static 	int	newPlaylist;
//static int shouldFillKeys;
static int local_flag;
static volatile int mPopupAnimation=0;


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

#pragma mark -

#include "MiniPlayerImplementTableView.h"
#include "AlertsCommonFunctions.h"
#include "PlaylistCommonFunctions.h"
#include "WaitingViewCommonMethods.h"

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
    [super setEditing:editing animated:animated];
    [tableView setEditing:editing animated:animated];
    if (editing==FALSE) {
        if (mDetailPlayerMode) self.navigationItem.rightBarButtonItem = nil;
        else {
        UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
        [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
        btn.adjustsImageWhenHighlighted = YES;
        [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
        UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithCustomView: btn];
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
    
    UIAlertAction *saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Save",@"") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
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
            childController.view.frame=self.view.frame;
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
    UIAlertController *alertC = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Warning",@"")
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
- (void)sortAZPlaylist {
    if (playlist->nb_entries) {
        
        NSString *tmpFP;
        if (!(playlist->playlist_id)) {
            tmpFP=[NSString stringWithString:detailViewController.mPlaylist[detailViewController.mPlaylist_pos].mPlaylistFilepath];
        }
                    
        qsort(playlist->entries,playlist->nb_entries,sizeof(t_playlist_entry),qsort_ComparePlaylistEntries);
        
        
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
- (void)sortZAPlaylist {
    if (playlist->nb_entries) {
        NSString *tmpFP;
        if (!(playlist->playlist_id)) {
            tmpFP=[NSString stringWithString:detailViewController.mPlaylist[detailViewController.mPlaylist_pos].mPlaylistFilepath];
        }
        
        qsort(playlist->entries,playlist->nb_entries,sizeof(t_playlist_entry),qsort_ComparePlaylistEntriesRev);
        
        
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
            int csection=indexPath.section;
            NSString *str=nil;
            if (show_playlist) {
                if (playlist->playlist_id==nil) crow++;
                crow-=2;
                str=playlist->entries[crow].fullpath;
            } else if (browse_depth>0) {
                csection-=2;
                t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
                str=cur_local_entries[csection][crow].fullpath;
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


- (void)viewDidLoad {
	clock_t start_time,end_time;
	start_time=clock();
	childController=NULL;
    
    self.navigationController.delegate = self;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    mFileMngr=[[NSFileManager alloc] init];
	
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
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithCustomView: btn];
    self.navigationItem.rightBarButtonItem = item;
    }
    if (show_playlist) {
        sBar.frame=CGRectMake(0,0,0,0);
        sBar.hidden=TRUE;
        //[self.view setNeedsUpdateConstraints];
        //[self.view setNeedsLayout];
    }
	
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
    if (browse_depth==0) {
        keys = [[NSMutableArray alloc] init];
        list = [[NSMutableArray alloc] init];
        NSMutableArray *mode_entries = [[NSMutableArray alloc] init];
        NSMutableArray *mode_entries_details = [[NSMutableArray alloc] init];
        [mode_entries addObject:NSLocalizedString(@"Add a playlist...",@"")];
        [mode_entries_details addObject:NSLocalizedString(@"Create a new playlist",@"")];
        
        if (detailViewController.mPlaylist_size) {
            [mode_entries addObject:NSLocalizedString(@"Now playing...",@"")];
            if (detailViewController.mPlaylist_size==1) [mode_entries_details addObject:NSLocalizedString(@"1 entry",@"")];
            else [mode_entries_details addObject:[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),detailViewController.mPlaylist_size]];
        }
        else {
            [mode_entries addObject:NSLocalizedString(@"Now playing...",@"")];
            [mode_entries_details addObject:NSLocalizedString(@"No entry",@"")];
        }
        
        [mode_entries addObject:NSLocalizedString(@"Random picks",@"")];
        [mode_entries_details addObject:NSLocalizedString(@"Generate a randomized playlist",@"")];
                
        [mode_entries addObject:NSLocalizedString(@"Most played...",@"")];
        [mode_entries_details addObject:[NSString stringWithFormat:@"%d entries",[self getMostPlayedCountFromDB]]];
                
        [mode_entries addObject:NSLocalizedString(@"Favorites...",@"")];
        [mode_entries_details addObject:[NSString stringWithFormat:@"%d entries",[self getFavoritesCountFromDB]]];
        
        [self loadPlayListsListFromDB:mode_entries list_id:list entries_details:mode_entries_details];
        NSDictionary *mode_entriesDict = [NSDictionary dictionaryWithObjectsAndKeys:mode_entries,@"entries",mode_entries_details,@"entries_details", nil];
        //NSDictionary *mode_entries_detailsDict = [NSDictionary dictionaryWithObject:mode_entries_details forKey:@"entries_details"];
        [keys addObject:mode_entriesDict];
        //[keys addObject:mode_entries_detailsDict];
    } else if (show_playlist) {
        switch (integrated_playlist) {
            case 0: //not integrated playlist, refresh currently selected playlist
                [self loadPlayListsFromDB:playlist->playlist_id intoPlaylist:playlist];
                break;
            case INTEGRATED_PLAYLIST_NOWPLAYING:
                [self reloadNowPlaying];
                break;
            case INTEGRATED_PLAYLIST_FAVORITES:
                [self loadFavoritesList];
                break;
            case INTEGRATED_PLAYLIST_MOSTPLAYED:
                [self loadMostPlayedList];
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

-(void) loadPlayListsListFromDB:(NSMutableArray*)entries list_id:(NSMutableArray*)list_id entries_details:(NSMutableArray*)details {
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
        
		
		sprintf(sqlStatement,"SELECT id,name,num_files FROM playlists ORDER BY name");
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				[entries addObject:[NSString stringWithFormat:@"%s",sqlite3_column_text(stmt, 1)]];
				[list_id addObject:[NSString stringWithFormat:@"%d",sqlite3_column_int(stmt, 0)]];
                if (sqlite3_column_int(stmt, 2)==1) [details addObject:@"1 entry"];
                else [details addObject:[NSString stringWithFormat:@"%d entries",sqlite3_column_int(stmt, 2)]];
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) loadPlayListsFromDB:(NSString *)_id_playlist intoPlaylist:(t_playlist *)_playlist  {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	
	pthread_mutex_lock(&db_mutex);
	_playlist->nb_entries=0;
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
                _playlist->playlist_name=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)];
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		sprintf(sqlStatement,"SELECT p.name,p.fullpath,s.rating,s.play_count FROM playlists_entries p \
				LEFT OUTER JOIN user_stats s ON p.fullpath=s.fullpath AND p.name=s.name \
				WHERE id_playlist=%s",[_id_playlist UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				_playlist->entries[_playlist->nb_entries].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
				_playlist->entries[_playlist->nb_entries].fullpath=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
				signed char tmpsc=(signed char)sqlite3_column_int(stmt, 2);
				if (tmpsc<0) tmpsc=0;
				if (tmpsc>5) tmpsc=5;
				_playlist->entries[_playlist->nb_entries].ratings=tmpsc;
				_playlist->entries[_playlist->nb_entries].playcounts=(short int)sqlite3_column_int(stmt, 3);
				_playlist->nb_entries++;
				if (_playlist->nb_entries>=MAX_PL_ENTRIES) break;
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(NSString *) minitNewPlaylistDB:(NSString *)listName {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	NSString *id_playlist;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		int err;
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		sprintf(sqlStatement,"INSERT INTO playlists (name,num_files) SELECT \"%s\",0",[listName UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
		
		//Get id
		id_playlist=[[NSString alloc] initWithFormat:@"%lld",sqlite3_last_insert_rowid(db) ];
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return id_playlist;
}
-(NSString *) getPlaylistNameDB:(NSString*)id_playlist {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	NSString *listName;
	sqlite3 *db;
	int err;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
		
		//Get playlist name
		sprintf(sqlStatement,"SELECT name FROM playlists WHERE id=%s",[id_playlist UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				listName=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return listName;
}

-(bool) addListToPlaylistDB {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	bool result;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		for (int i=0;i<playlist->nb_entries;i++) {
			sprintf(sqlStatement,"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
					[playlist->playlist_id UTF8String],[playlist->entries[i].label UTF8String],[playlist->entries[i].fullpath UTF8String]);
			err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
			if (err==SQLITE_OK){
				result=TRUE;
			} else {
				result=FALSE;
				NSLog(@"ErrSQL : %d",err);
				break;
			}
		}
		if (result) {
			sprintf(sqlStatement,"UPDATE playlists SET num_files=\
					(SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
					WHERE id=%s",
					[playlist->playlist_id UTF8String],[playlist->playlist_id UTF8String]);
			err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
			if (err==SQLITE_OK){
				result=TRUE;
			} else {
				result=FALSE;
				NSLog(@"ErrSQL : %d",err);
			}
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return result;
}
-(bool) addListToPlaylistDB:(NSString*)id_playlist entries:(t_plPlaylist_entry*)pl_entries nb_entries:(int)nb_entries  {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	bool result;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		for (int i=0;i<nb_entries;i++) {
			sprintf(sqlStatement,"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
					[id_playlist UTF8String],[pl_entries[i].mPlaylistFilename UTF8String],[pl_entries[i].mPlaylistFilepath UTF8String]);
			err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
			if (err==SQLITE_OK){
				result=TRUE;
			} else {
				result=FALSE;
				NSLog(@"ErrSQL : %d",err);
				break;
			}
		}
		if (result) {
			sprintf(sqlStatement,"UPDATE playlists SET num_files=\
					(SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
					WHERE id=%s",
					[id_playlist UTF8String],[id_playlist UTF8String]);
			err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
			if (err==SQLITE_OK){
				result=TRUE;
			} else {
				result=FALSE;
				NSLog(@"ErrSQL : %d",err);
			}
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return result;
}
-(bool) removeFromPlaylistDB:(NSString*)id_playlist fullPath:(NSString*)fullpath {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	bool result;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
		
		sprintf(sqlStatement,"DELETE FROM playlists_entries WHERE id_playlist=\"%s\" AND fullpath=\"%s\"",
				[id_playlist UTF8String],[fullpath UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
			result=TRUE;
		} else {
			NSLog(@"ErrSQL : %d",err);
			result=FALSE;
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return result;
}
-(bool) replacePlaylistDBwithCurrent {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		sprintf(sqlStatement,"DELETE FROM playlists_entries WHERE id_playlist=%s",
				[playlist->playlist_id UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
		
		err=sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		for (int i=0;i<playlist->nb_entries;i++) {
			sprintf(sqlStatement,"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
					[playlist->playlist_id UTF8String],[playlist->entries[i].label UTF8String],[playlist->entries[i].fullpath UTF8String]);
			err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
			if (err==SQLITE_OK){
			} else NSLog(@"ErrSQL : %d",err);
		}
        
        err=sqlite3_exec(db, "COMMIT", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
		
		sprintf(sqlStatement,"UPDATE playlists SET num_files=\
				(SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
				WHERE id=%s",
				[playlist->playlist_id UTF8String],[playlist->playlist_id UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return TRUE;
}
-(void) updatePlaylistNameDB:(NSString*)id_playlist playlist_name:(NSString *)playlist_name {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		sprintf(sqlStatement,"UPDATE playlists SET name=\"%s\" WHERE id=%s",[playlist_name UTF8String],[id_playlist UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}

-(int) deletePlaylistDB:(NSString*)id_playlist {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err,ret;
	pthread_mutex_lock(&db_mutex);
	ret=1;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
		sprintf(sqlStatement,"DELETE FROM playlists_entries WHERE id_playlist=%s",[id_playlist UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else {ret=0;NSLog(@"ErrSQL : %d",err);}
		
		sprintf(sqlStatement,"DELETE FROM playlists WHERE id=%s",[id_playlist UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else {ret=0;NSLog(@"ErrSQL : %d",err);}
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return ret;
}


-(void)listLocalFiles {
	NSString *file,*cpath;
	NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
	NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
	NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
    NSArray *filetype_extATARISOUND=[SUPPORTED_FILETYPE_ATARISOUND componentsSeparatedByString:@","];
	NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
    NSArray *filetype_extPT3=[SUPPORTED_FILETYPE_PT3 componentsSeparatedByString:@","];
    NSArray *filetype_extPIXEL=[SUPPORTED_FILETYPE_PIXEL componentsSeparatedByString:@","];
	NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
	NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
	NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","];
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
	NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
	NSArray *filetype_ext2SF=[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","];
    NSArray *filetype_extV2M=[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSArray *filetype_extHC=[SUPPORTED_FILETYPE_HC componentsSeparatedByString:@","];
    NSArray *filetype_extEUP=[SUPPORTED_FILETYPE_EUP componentsSeparatedByString:@","];
	NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extS98=[SUPPORTED_FILETYPE_S98 componentsSeparatedByString:@","];
    NSArray *filetype_extKSS=[SUPPORTED_FILETYPE_KSS componentsSeparatedByString:@","];
	NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
	NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
	NSMutableArray *filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extMDX count]+[filetype_extSID count]+[filetype_extSTSOUND count]+[filetype_extATARISOUND count]+
								  [filetype_extSC68 count]+[filetype_extPT3 count]+[filetype_extPIXEL count]+[filetype_extARCHIVE count]+[filetype_extUADE count]+[filetype_extMODPLUG count]+[filetype_extXMP count]+
								  [filetype_extGME count]+[filetype_extADPLUG count]+[filetype_ext2SF count]+[filetype_extV2M count]+[filetype_extVGMSTREAM count]+
								  [filetype_extHC count]+[filetype_extEUP count]+[filetype_extHVL count]+[filetype_extS98 count]+[filetype_extKSS count]+[filetype_extGSF count]+
								  [filetype_extASAP count]+[filetype_extWMIDI count]+[filetype_extVGM count]];
    NSArray *filetype_extARCHIVEFILE=[SUPPORTED_FILETYPE_ARCFILE componentsSeparatedByString:@","];
	NSMutableArray *archivetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extARCHIVEFILE count]];
	NSArray *filetype_extGME_MULTISONGSFILE=[SUPPORTED_FILETYPE_GME_MULTISONGS componentsSeparatedByString:@","];
	NSMutableArray *gme_multisongstype_ext=[NSMutableArray arrayWithCapacity:[filetype_extGME_MULTISONGSFILE count]];
    NSArray *filetype_extSID_MULTISONGSFILE=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
	NSMutableArray *sid_multisongstype_ext=[NSMutableArray arrayWithCapacity:[filetype_extSID_MULTISONGSFILE count]];
    
    NSMutableArray *all_multisongstype_ext=[NSMutableArray arrayWithCapacity:[filetype_extGME_MULTISONGSFILE count]+[filetype_extSID_MULTISONGSFILE count]];
	
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	char sqlStatement[1024];
	sqlite3_stmt *stmt;
	int local_entries_index,local_nb_entries_limit;
    int browseType;
    int shouldStop=0;
	
	NSRange r;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
    
    if (search_local_nb_entries) {
        for (int i=0;i<27;i++) {
            for (int j=0;j<search_local_entries_count[i];j++) {
                search_local_entries[i][j].label=nil;
                search_local_entries[i][j].fullpath=nil;
            }
            search_local_entries[i]=NULL;
        }
        search_local_nb_entries=0;
        free(search_local_entries_data);
    }
    
	search_local=0;
	if (mSearch) {
		search_local=1;
		
		search_local_entries_data=(t_local_browse_entry*)calloc(local_nb_entries,sizeof(t_local_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_local_entries_count[i]=0;
			if (local_entries_count[i]) search_local_entries[i]=&(search_local_entries_data[search_local_nb_entries]);
			for (int j=0;j<local_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [local_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_local_entries[i][search_local_entries_count[i]].label=local_entries[i][j].label;
					search_local_entries[i][search_local_entries_count[i]].fullpath=local_entries[i][j].fullpath;
					search_local_entries[i][search_local_entries_count[i]].playcount=local_entries[i][j].playcount;
					search_local_entries[i][search_local_entries_count[i]].rating=local_entries[i][j].rating;
					search_local_entries[i][search_local_entries_count[i]].type=local_entries[i][j].type;
					
					search_local_entries[i][search_local_entries_count[i]].song_length=local_entries[i][j].song_length;
					search_local_entries[i][search_local_entries_count[i]].songs=local_entries[i][j].songs;
					search_local_entries[i][search_local_entries_count[i]].channels_nb=local_entries[i][j].channels_nb;
                    
					search_local_entries_count[i]++;
					search_local_nb_entries++;
				}
			}
		}
		return;
	}
	
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) != SQLITE_OK) db=NULL;
    
    err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
    if (err==SQLITE_OK){
    } else NSLog(@"ErrSQL : %d",err);
	
	[filetype_ext addObjectsFromArray:filetype_extMDX];
    [filetype_ext addObjectsFromArray:filetype_extPMD];
	[filetype_ext addObjectsFromArray:filetype_extSID];
	[filetype_ext addObjectsFromArray:filetype_extSTSOUND];
    [filetype_ext addObjectsFromArray:filetype_extATARISOUND];
	[filetype_ext addObjectsFromArray:filetype_extSC68];
    [filetype_ext addObjectsFromArray:filetype_extPT3];
	[filetype_ext addObjectsFromArray:filetype_extARCHIVE];
	[filetype_ext addObjectsFromArray:filetype_extUADE];
	[filetype_ext addObjectsFromArray:filetype_extMODPLUG];
    [filetype_ext addObjectsFromArray:filetype_extXMP];
    [filetype_ext addObjectsFromArray:filetype_extGME];
	[filetype_ext addObjectsFromArray:filetype_extADPLUG];
	[filetype_ext addObjectsFromArray:filetype_ext2SF];
    [filetype_ext addObjectsFromArray:filetype_extV2M];
    [filetype_ext addObjectsFromArray:filetype_extVGMSTREAM];
    [filetype_ext addObjectsFromArray:filetype_extHC];
    [filetype_ext addObjectsFromArray:filetype_extEUP];
	[filetype_ext addObjectsFromArray:filetype_extHVL];
    [filetype_ext addObjectsFromArray:filetype_extS98];
    [filetype_ext addObjectsFromArray:filetype_extKSS];
	[filetype_ext addObjectsFromArray:filetype_extGSF];
	[filetype_ext addObjectsFromArray:filetype_extASAP];
	[filetype_ext addObjectsFromArray:filetype_extWMIDI];
    [filetype_ext addObjectsFromArray:filetype_extVGM];
    
    [archivetype_ext addObjectsFromArray:filetype_extARCHIVEFILE];
    [gme_multisongstype_ext addObjectsFromArray:filetype_extGME_MULTISONGSFILE];
    [sid_multisongstype_ext addObjectsFromArray:filetype_extSID_MULTISONGSFILE];
    
    [all_multisongstype_ext addObjectsFromArray:filetype_extGME_MULTISONGSFILE];
    [all_multisongstype_ext addObjectsFromArray:filetype_extSID_MULTISONGSFILE];
	
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {
            local_entries_data[i].label=nil;
            local_entries_data[i].fullpath=nil;
        }
        for (int i=0;i<27;i++) {
            for (int j=0;j<local_entries_count[i];j++) {
                local_entries[i][j].label=nil;
                local_entries[i][j].fullpath=nil;
            }
            local_entries[i]=NULL;
        }
        free(local_entries_data);local_entries_data=NULL;
        local_nb_entries=0;
    }
    for (int i=0;i<27;i++) local_entries_count[i]=0;
	
	// First check count for each section
	cpath=[NSHomeDirectory() stringByAppendingPathComponent:  currentPath];
    //NSLog(@"%@\n%@",cpath,currentPath);
    //Check if it is a directory or an archive
    BOOL isDirectory;
    browseType=0;
    if ([mFileMngr fileExistsAtPath:cpath isDirectory:&isDirectory]) {
        if (!isDirectory) {
            //file:check if archive or multisongs
            NSString *extension=[[[cpath lastPathComponent] pathExtension] uppercaseString];
            if ([archivetype_ext indexOfObject:extension]!=NSNotFound) browseType=1;
            //check if Multisongs file
            else if ([gme_multisongstype_ext indexOfObject:extension]!=NSNotFound) browseType=2;
            else if ([sid_multisongstype_ext indexOfObject:extension]!=NSNotFound) browseType=3;
        }
    }
    
    if (browseType==3) {//SID
        SidTune *mSidTune=new SidTune([cpath UTF8String],0,true);
        
        if (mSidTune==NULL) {
            NSLog(@"SID SidTune init error");
            if (mSidTune) {delete mSidTune;mSidTune=NULL;}
        } else {
            const SidTuneInfo *sidtune_info;
            sidtune_info=mSidTune->getInfo();
            
            for (int i=0;i<sidtune_info->songs();i++){
                const SidTuneInfo *s_info;
                file=nil;
                mSidTune->selectSong(i);
                s_info=mSidTune->getInfo();
                
                if (s_info->infoString(0)[0]) {
                    file=[NSString stringWithFormat:@"%.3d-%s",i,s_info->infoString(0)];
                } else {
                    file=[NSString stringWithFormat:@"%.3d-%@",i,[cpath lastPathComponent]];
                }
                int filtered=0;
                if ((mSearch)&&([mSearchText length]>0)) {
                    filtered=1;
                    NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                    if (r.location != NSNotFound) {
                        /*if(r.location== 0)*/ filtered=0;
                    }
                }
                if (!filtered) {
                    
                    const char *str=[file UTF8String];
                    int index=0;
                    if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                    if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                    local_entries_count[index]++;
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
                        [self showAlertMsg:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"")];
                    } else {
                        //show alert : limited list
                        [self showAlertMsg:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                        local_nb_entries=local_nb_entries_limit;
                    }
                } else local_nb_entries_limit=0;
                if (local_entries_data) {
                    local_entries_index=0;
                    for (int i=0;i<27;i++)
                        if (local_entries_count[i]) {
                            if (local_entries_index+local_entries_count[i]>local_nb_entries) {
                                local_entries_count[i]=local_nb_entries-local_entries_index;
                                local_entries[i]=&(local_entries_data[local_entries_index]);
                                local_entries_index+=local_entries_count[i];
                                local_entries_count[i]=0;
                                for (int j=i+1;j<27;j++) local_entries_count[i]=0;
                            } else {
                                local_entries[i]=&(local_entries_data[local_entries_index]);
                                local_entries_index+=local_entries_count[i];
                                local_entries_count[i]=0;
                            }
                        }
                    
                    for (int i=0;i<sidtune_info->songs();i++){
                        const SidTuneInfo *s_info;
                        file=nil;
                        mSidTune->selectSong(i);
                        s_info=mSidTune->getInfo();
                        
                        if (s_info->infoString(0)[0]) {
                            file=[NSString stringWithFormat:@"%.3d-%s",i,s_info->infoString(0)];
                        } else {
                            file=[NSString stringWithFormat:@"%.3d-%@",i,[cpath lastPathComponent]];
                        }
                        
                        int filtered=0;
                        if ((mSearch)&&([mSearchText length]>0)) {
                            filtered=1;
                            NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                            if (r.location != NSNotFound) {
                                /*if(r.location== 0)*/ filtered=0;
                            }
                        }
                        if (!filtered) {
                            
                            const char *str;
                            str=[file UTF8String];
                            int index=0;
                            if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                            if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                            local_entries[index][local_entries_count[index]].type=1;
                            local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                            local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@?%d",currentPath,i];
                            
                            local_entries[index][local_entries_count[index]].rating=0;
                            local_entries[index][local_entries_count[index]].playcount=0;
                            local_entries[index][local_entries_count[index]].song_length=0;
                            local_entries[index][local_entries_count[index]].songs=1;//0;
                            local_entries[index][local_entries_count[index]].channels_nb=0;
                            
                            sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[[file lastPathComponent] UTF8String],[local_entries[index][local_entries_count[index]].fullpath UTF8String]);
                            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                            if (err==SQLITE_OK){
                                while (sqlite3_step(stmt) == SQLITE_ROW) {
                                    signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                    if (rating<0) rating=0;
                                    if (rating>5) rating=5;
                                    local_entries[index][local_entries_count[index]].playcount=(short int)sqlite3_column_int(stmt, 0);
                                    local_entries[index][local_entries_count[index]].rating=rating;
                                    local_entries[index][local_entries_count[index]].song_length=(int)sqlite3_column_int(stmt, 2);
                                    local_entries[index][local_entries_count[index]].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                    //local_entries[index][local_entries_count[index]].songs=(int)sqlite3_column_int(stmt, 4);
                                }
                                sqlite3_finalize(stmt);
                            } else NSLog(@"ErrSQL : %d",err);
                            
                            local_entries_count[index]++;
                            
                            if (local_nb_entries_limit) {
                                local_nb_entries_limit--;
                                if (!local_nb_entries_limit) shouldStop=1;
                            }
                            
                        }
                    }
                }
            }
            if (mSidTune) {delete mSidTune;mSidTune=NULL;}
        }
    } else if (browseType==2) { //GME Multisongs
        // Open music file in new emulator
        Music_Emu* gme_emu;
        
        gme_err_t gme_err=gme_open_file( [cpath UTF8String], &gme_emu, gme_info_only );
        if (gme_err) {
            NSLog(@"gme_open_file error: %s",gme_err);
        } else {
            gme_info_t *gme_info;
            
            //is a m3u available ?
            NSString *tmpStr=[NSString stringWithFormat:@"%@.m3u",[cpath stringByDeletingPathExtension]];
            gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
            if (gme_err) {
                NSString *tmpStr=[NSString stringWithFormat:@"%@.M3U",[cpath stringByDeletingPathExtension]];
                gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
            }
            
            for (int i=0;i<gme_track_count( gme_emu );i++) {
                if (gme_track_info( gme_emu, &gme_info, i )==0) {
                    file=nil;
                    if (gme_info->song) {
                        if (gme_info->song[0]) file=[NSString stringWithFormat:@"%s",gme_info->song];
                    }
                    if (!file) {
                        if (gme_info->game) {
                            if (gme_info->game[0]) file=[NSString stringWithFormat:@"%.3d-%s",i,gme_info->game];
                        }
                    }
                    if (!file) {
                        file=[NSString stringWithFormat:@"%.3d-%@",i,[cpath lastPathComponent]];
                    }
                    
                    int filtered=0;
                    if ((mSearch)&&([mSearchText length]>0)) {
                        filtered=1;
                        NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                        if (r.location != NSNotFound) {
                            /*if(r.location== 0)*/ filtered=0;
                        }
                    }
                    if (!filtered) {
                        
                        const char *str=[file UTF8String];
                        int index=0;
                        if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                        if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                        local_entries_count[index]++;
                        local_nb_entries++;
                    }
                    gme_free_info(gme_info);
                }
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
                    [self showAlertMsg:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"")];
                } else {
                    //show alert : limited list
                    [self showAlertMsg:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                for (int i=0;i<27;i++)
                    if (local_entries_count[i]) {
                        if (local_entries_index+local_entries_count[i]>local_nb_entries) {
                            local_entries_count[i]=local_nb_entries-local_entries_index;
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                            for (int j=i+1;j<27;j++) local_entries_count[i]=0;
                        } else {
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                        }
                    }
                
                gme_err_t gme_err=gme_open_file( [cpath UTF8String], &gme_emu, gme_info_only );
                if (gme_err) {
                    NSLog(@"gme_open_file error: %s",gme_err);
                } else {
                    gme_info_t *gme_info;
                    
                    //is a m3u available ?
                    NSString *tmpStr=[NSString stringWithFormat:@"%@.m3u",[cpath stringByDeletingPathExtension]];
                    gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
                    if (gme_err) {
                        NSString *tmpStr=[NSString stringWithFormat:@"%@.M3U",[cpath stringByDeletingPathExtension]];
                        gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
                    }
                    
                    for (int i=0;i<gme_track_count( gme_emu );i++) {
                        if (gme_track_info( gme_emu, &gme_info, i )==0) {
                            file=nil;
                            if (gme_info->song) {
                                if (gme_info->song[0]) file=[NSString stringWithFormat:@"%s",gme_info->song];
                            }
                            if (!file) {
                                if (gme_info->game) {
                                    if (gme_info->game[0]) file=[NSString stringWithFormat:@"%.3d-%s",i,gme_info->game];
                                }
                            }
                            if (!file) {
                                file=[NSString stringWithFormat:@"%.3d-%@",i,[cpath lastPathComponent]];
                            }
                            
                            int filtered=0;
                            if ((mSearch)&&([mSearchText length]>0)) {
                                filtered=1;
                                NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                if (r.location != NSNotFound) {
                                    /*if(r.location== 0)*/ filtered=0;
                                }
                            }
                            if (!filtered) {
                                
                                const char *str;
                                str=[file UTF8String];
                                int index=0;
                                if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                local_entries[index][local_entries_count[index]].type=1;
                                local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                                local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@?%d",currentPath,i];
                                
                                local_entries[index][local_entries_count[index]].rating=0;
                                local_entries[index][local_entries_count[index]].playcount=0;
                                local_entries[index][local_entries_count[index]].song_length=0;
                                local_entries[index][local_entries_count[index]].songs=1;//0;
                                local_entries[index][local_entries_count[index]].channels_nb=0;
                                
                                sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[[file lastPathComponent] UTF8String],[local_entries[index][local_entries_count[index]].fullpath UTF8String]);
                                err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                                if (err==SQLITE_OK){
                                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                                        signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                        if (rating<0) rating=0;
                                        if (rating>5) rating=5;
                                        local_entries[index][local_entries_count[index]].playcount=(short int)sqlite3_column_int(stmt, 0);
                                        local_entries[index][local_entries_count[index]].rating=rating;
                                        local_entries[index][local_entries_count[index]].song_length=(int)sqlite3_column_int(stmt, 2);
                                        local_entries[index][local_entries_count[index]].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                        //local_entries[index][local_entries_count[index]].songs=(int)sqlite3_column_int(stmt, 4);
                                    }
                                    sqlite3_finalize(stmt);
                                } else NSLog(@"ErrSQL : %d",err);
                                
                                local_entries_count[index]++;
                                
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
    } else if (browseType==1) { //FEX Archive (zip,7z,rar,rsn)
        fex_type_t type;
        fex_t* fex;
        const char *path=[cpath UTF8String];
        /* Determine file's type */
        if (fex_identify_file( &type, path)) {
            NSLog(@"fex cannot determine type of %s",path);
        }
        /* Only open files that fex can handle */
        if ( type != NULL ) {
            if (fex_open_type( &fex, path, type )) {
                NSLog(@"cannot fex open : %s",path);
            } else {
                while ( !fex_done( fex ) ) {
                    file=[NSString stringWithFormat:@"%s",fex_name(fex)];
                    NSString *extension = [[file pathExtension] uppercaseString];
                    
                    NSString *file_no_ext = [[[[file lastPathComponent] componentsSeparatedByString:@"."] firstObject] uppercaseString];
                    
                    int filtered=0;
                    if ((mSearch)&&([mSearchText length]>0)) {
                        filtered=1;
                        NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                        if (r.location != NSNotFound) {
                            /*if(r.location== 0)*/ filtered=0;
                        }
                    }
                    if (!filtered) {
                        int found=0;
                        
                        if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                        else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                        
                        if (found)  {
                            const char *str=[[file lastPathComponent] UTF8String];
                            int index=0;
                            if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                            if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                            local_entries_count[index]++;
                            local_nb_entries++;
                        }
                    }
                    
                    if (fex_next( fex )) {
                        NSLog(@"Error during fex scanning");
                        break;
                    }
                }
                fex_close( fex );
            }
            fex = NULL;
        } else {
            //NSLog( @"Skipping unsupported archive: %s\n", path );
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
                    [self showAlertMsg:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"")];
                } else {
                    //show alert : limited list
                    [self showAlertMsg:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                for (int i=0;i<27;i++)
                    if (local_entries_count[i]) {
                        if (local_entries_index+local_entries_count[i]>local_nb_entries) {
                            local_entries_count[i]=local_nb_entries-local_entries_index;
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                            for (int j=i+1;j<27;j++) local_entries_count[i]=0;
                        } else {
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                        }
                    }
                if (fex_open_type( &fex, path, type )) {
                    NSLog(@"cannot fex open : %s",path);
                } else {
                    int arc_counter=0;
                    while ( !fex_done( fex ) ) {
                        file=[NSString stringWithFormat:@"%s",fex_name(fex)];
                        NSString *extension = [[file pathExtension] uppercaseString];
                        NSString *file_no_ext = [[[[file lastPathComponent] componentsSeparatedByString:@"."] firstObject] uppercaseString];
                        
                        int filtered=0;
                        if ((mSearch)&&([mSearchText length]>0)) {
                            filtered=1;
                            NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                            if (r.location != NSNotFound) {
                                /*if(r.location== 0)*/ filtered=0;
                            }
                        }
                        if (!filtered) {
                            int found=0;
                            
                            if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                            else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                            
                            if (found)  {
                                const char *str;
                                char tmp_str[1024];//,*tmp_convstr;
                                int toto=0;
                                str=[[file lastPathComponent] UTF8String];
                                if ([extension caseInsensitiveCompare:@"mdx"]==NSOrderedSame ) {
                                    [[file lastPathComponent] getFileSystemRepresentation:tmp_str maxLength:1024];
                                    //tmp_convstr=mdx_make_sjis_to_syscharset(tmp_str);
                                    toto=1;
                                }
                                int index=0;
                                if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                local_entries[index][local_entries_count[index]].type=1;
                                //check if Archive file
                                if ([archivetype_ext indexOfObject:extension]!=NSNotFound) local_entries[index][local_entries_count[index]].type=2;
                                else if ([archivetype_ext indexOfObject:file_no_ext]!=NSNotFound) local_entries[index][local_entries_count[index]].type=2;
                                //check if Multisongs file
                                if (toto) {
                                    local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithCString:tmp_str encoding:NSUTF8StringEncoding];
                                    //	free(tmp_convstr);
                                } else local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                                
                                local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@@%d",currentPath,arc_counter];
                                
                                local_entries[index][local_entries_count[index]].rating=0;
                                local_entries[index][local_entries_count[index]].playcount=0;
                                local_entries[index][local_entries_count[index]].song_length=0;
                                local_entries[index][local_entries_count[index]].songs=0;
                                local_entries[index][local_entries_count[index]].channels_nb=0;
                                
                                sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[[file lastPathComponent] UTF8String],[local_entries[index][local_entries_count[index]].fullpath UTF8String]);
                                err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                                if (err==SQLITE_OK){
                                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                                        signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                        if (rating<0) rating=0;
                                        if (rating>5) rating=5;
                                        local_entries[index][local_entries_count[index]].playcount=(short int)sqlite3_column_int(stmt, 0);
                                        local_entries[index][local_entries_count[index]].rating=rating;
                                        local_entries[index][local_entries_count[index]].song_length=(int)sqlite3_column_int(stmt, 2);
                                        local_entries[index][local_entries_count[index]].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                        local_entries[index][local_entries_count[index]].songs=(int)sqlite3_column_int(stmt, 4);
                                    }
                                    sqlite3_finalize(stmt);
                                } else NSLog(@"ErrSQL : %d",err);
                                
                                local_entries_count[index]++;
                                arc_counter++;
                                
                                if (local_nb_entries_limit) {
                                    local_nb_entries_limit--;
                                    if (!local_nb_entries_limit) shouldStop=1;
                                }
                            }
                        }
                        if (fex_next( fex )) {
                            NSLog(@"Error during fex scanning");
                            break;
                        }
                    }
                    fex_close( fex );
                }
                fex = NULL;
            }
            
        }
    } else {
        //        clock_t start_time,end_time;
        //        start_time=clock();
        NSError *error;
        NSRange rdir;
        NSArray *dirContent;//
        BOOL isDir;
        if (mShowSubdir) dirContent=[mFileMngr subpathsOfDirectoryAtPath:cpath error:&error];
        else dirContent=[mFileMngr contentsOfDirectoryAtPath:cpath error:&error];
        for (file in dirContent) {
            //check if dir
            //rdir.location=NSNotFound;
            //rdir = [file rangeOfString:@"." options:NSCaseInsensitiveSearch];
            [mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
            if (isDir) { //rdir.location == NSNotFound) {  //assume it is a dir if no "." in file name
                rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                    if (1/*[file compare:@"tmpArchive"]!=NSOrderedSame*/) {
                        //do not display dir if subdir mode is on
                        int filtered=mShowSubdir;
                        if (!filtered) {
                            if ((mSearch)&&([mSearchText length]>0)) {
                                filtered=1;
                                NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                if (r.location != NSNotFound) {
                                    /*if(r.location== 0)*/ filtered=0;
                                }
                            }
                            if (!filtered) {
                                const char *str=[file UTF8String];
                                int index=0;
                                if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                local_entries_count[index]++;
                                local_nb_entries++;
                            }
                        }
                    }
                }
            } else {
                rdir.location=NSNotFound;
                rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                    NSString *extension = [[file pathExtension] uppercaseString];
                    NSString *file_no_ext = [[[[file lastPathComponent] componentsSeparatedByString:@"."] firstObject] uppercaseString];
                    
                    int filtered=0;
                    if ((mSearch)&&([mSearchText length]>0)) {
                        filtered=1;
                        NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                        if (r.location != NSNotFound) {
                            /*if(r.location== 0)*/ filtered=0;
                        }
                    }
                    if (!filtered) {
                        int found=0;
                        
                        if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                        else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                        
                        if (found)  {
                            const char *str=[[file lastPathComponent] UTF8String];
                            int index=0;
                            if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                            if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                            local_entries_count[index]++;
                            local_nb_entries++;
                        }
                    }
                }
            }
        }
        //        end_time=clock();
        //        NSLog(@"detail1 : %d",end_time-start_time);
        //        start_time=end_time;
        
        
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
                    [self showAlertMsg:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"")];
                } else {
                    //show alert : limited list
                    [self showAlertMsg:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                for (int i=0;i<27;i++)
                    if (local_entries_count[i]) {
                        if (local_entries_index+local_entries_count[i]>local_nb_entries) {
                            local_entries_count[i]=local_nb_entries-local_entries_index;
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                            for (int j=i+1;j<27;j++) local_entries_count[i]=0;
                        } else {
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                        }
                    }
                
                //                end_time=clock();
                //                NSLog(@"detail2 : %d",end_time-start_time);
                //                start_time=end_time;
                // Second check count for each section
                for (file in dirContent) {
                    if (shouldStop) break;
                    //rdir.location=NSNotFound;
                    // rdir = [file rangeOfString:@"." options:NSCaseInsensitiveSearch];
                    [mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
                    if (isDir) { //rdir.location == NSNotFound) {  //assume it is a dir if no "." in file name
                        rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                        if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                            if (1/*[file compare:@"tmpArchive"]!=NSOrderedSame*/) {
                                //do not display dir if subdir mode is on
                                int filtered=mShowSubdir;
                                if (!filtered) {
                                    if ((mSearch)&&([mSearchText length]>0)) {
                                        filtered=1;
                                        NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                        if (r.location != NSNotFound) {
                                            /*if(r.location== 0)*/ filtered=0;
                                        }
                                    }
                                    if (!filtered) {
                                        const char *str=[file UTF8String];
                                        int index=0;
                                        if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                        if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                        local_entries[index][local_entries_count[index]].type=0;
                                        
                                        local_entries[index][local_entries_count[index]].label=[[NSString alloc] initWithString:file];
                                        local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@/%@",currentPath,file];
                                        local_entries_count[index]++;
                                        if (local_nb_entries_limit) {
                                            local_nb_entries_limit--;
                                            if (!local_nb_entries_limit) shouldStop=1;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        rdir.location=NSNotFound;
                        rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                        if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                            NSString *extension = [[file pathExtension] uppercaseString];
                            NSString *file_no_ext = [[[[file lastPathComponent] componentsSeparatedByString:@"."] firstObject] uppercaseString];
                            
                            int filtered=0;
                            if ((mSearch)&&([mSearchText length]>0)) {
                                filtered=1;
                                NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                if (r.location != NSNotFound) {
                                    /*if(r.location== 0)*/ filtered=0;
                                }
                            }
                            if (!filtered) {
                                int found=0;
                                
                                if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                                else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                                
                                
                                if (found)  {
                                    const char *str;
                                    char tmp_str[1024];//,*tmp_convstr;
                                    int toto=0;
                                    str=[[file lastPathComponent] UTF8String];
                                    if ([extension caseInsensitiveCompare:@"mdx"]==NSOrderedSame ) {
                                        [[file lastPathComponent] getFileSystemRepresentation:tmp_str maxLength:1024];
                                        //tmp_convstr=mdx_make_sjis_to_syscharset(tmp_str);
                                        toto=1;
                                    }
                                    int index=0;
                                    if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                    if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                    local_entries[index][local_entries_count[index]].type=1;
                                    //check if Archive file
                                    if ([archivetype_ext indexOfObject:extension]!=NSNotFound) local_entries[index][local_entries_count[index]].type=2;
                                    else if ([archivetype_ext indexOfObject:file_no_ext]!=NSNotFound) local_entries[index][local_entries_count[index]].type=2;
                                    //check if Multisongs file
                                    else if ([all_multisongstype_ext indexOfObject:extension]!=NSNotFound) local_entries[index][local_entries_count[index]].type=3;
                                    else if ([all_multisongstype_ext indexOfObject:file_no_ext]!=NSNotFound) local_entries[index][local_entries_count[index]].type=3;
                                    if (toto) {
                                        local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithCString:tmp_str encoding:NSUTF8StringEncoding];
                                        //	free(tmp_convstr);
                                    } else local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                                    
                                    local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@/%@",currentPath,file];
                                    
                                    local_entries[index][local_entries_count[index]].rating=0;
                                    local_entries[index][local_entries_count[index]].playcount=0;
                                    local_entries[index][local_entries_count[index]].song_length=0;
                                    local_entries[index][local_entries_count[index]].songs=0;
                                    local_entries[index][local_entries_count[index]].channels_nb=0;
                                    
                                    sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs FROM user_stats WHERE name=\"%s\" and fullpath=\"%s/%s\"",[[file lastPathComponent] UTF8String],[currentPath UTF8String],[file UTF8String]);
                                    err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                                    if (err==SQLITE_OK){
                                        while (sqlite3_step(stmt) == SQLITE_ROW) {
                                            signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                            if (rating<0) rating=0;
                                            if (rating>5) rating=5;
                                            local_entries[index][local_entries_count[index]].playcount=(short int)sqlite3_column_int(stmt, 0);
                                            local_entries[index][local_entries_count[index]].rating=rating;
                                            local_entries[index][local_entries_count[index]].song_length=(int)sqlite3_column_int(stmt, 2);
                                            local_entries[index][local_entries_count[index]].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                            local_entries[index][local_entries_count[index]].songs=(int)sqlite3_column_int(stmt, 4);
                                        }
                                        sqlite3_finalize(stmt);
                                    } else NSLog(@"ErrSQL : %d",err);
                                    
                                    local_entries_count[index]++;
                                    
                                    if (local_nb_entries_limit) {
                                        local_nb_entries_limit--;
                                        if (!local_nb_entries_limit) shouldStop=1;
                                    }
                                }
                            }
                        }
                    }
                }
                //                end_time=clock();
                //                NSLog(@"detail1 : %d",end_time-start_time);
            }
        }
    }
    
    if (db) {
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
    }
    
    
    return;
}

-(void) loadFavoritesList{
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    playlist->nb_entries=0;
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
            sprintf(sqlStatement,"SELECT name,fullpath,rating,play_count,length,channels,songs FROM user_stats WHERE rating=5 ORDER BY rating DESC,name");
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    playlist->entries[playlist->nb_entries].label=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
                    playlist->entries[playlist->nb_entries].fullpath=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
                    playlist->entries[playlist->nb_entries].ratings=(signed char)sqlite3_column_int(stmt,2);
                    
                    playlist->entries[playlist->nb_entries].playcounts=(short int)sqlite3_column_int(stmt,3);
                    playlist->entries[playlist->nb_entries].song_length=(int)sqlite3_column_int(stmt,4);
                    playlist->entries[playlist->nb_entries].channels_nb=(char)sqlite3_column_int(stmt,5);
                    playlist->entries[playlist->nb_entries].songs=(int)sqlite3_column_int(stmt,6);
                    playlist->nb_entries++;
                }
                sqlite3_finalize(stmt);
            } else NSLog(@"ErrSQL : %d",err);
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
}

-(void) loadMostPlayedList{
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    playlist->nb_entries=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
            sprintf(sqlStatement,"SELECT name,fullpath,rating,play_count,length,channels,songs FROM user_stats WHERE play_count>0 ORDER BY play_count DESC,name");
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    playlist->entries[playlist->nb_entries].label=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
                    playlist->entries[playlist->nb_entries].fullpath=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
                    playlist->entries[playlist->nb_entries].ratings=(signed char)sqlite3_column_int(stmt,2);

                    playlist->entries[playlist->nb_entries].playcounts=(short int)sqlite3_column_int(stmt,3);
                    playlist->entries[playlist->nb_entries].song_length=(int)sqlite3_column_int(stmt,4);
                    playlist->entries[playlist->nb_entries].channels_nb=(char)sqlite3_column_int(stmt,5);
                    playlist->entries[playlist->nb_entries].songs=(int)sqlite3_column_int(stmt,6);
                    playlist->nb_entries++;
                }
                sqlite3_finalize(stmt);
            } else NSLog(@"ErrSQL : %d",err);
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
}

-(void) reloadNowPlaying {
    for (int i=0;i<detailViewController.mPlaylist_size;i++) {
        playlist->entries[i].label=[[NSString alloc] initWithString:detailViewController.mPlaylist[i].mPlaylistFilename];
        playlist->entries[i].fullpath=[[NSString alloc ] initWithString:detailViewController.mPlaylist[i].mPlaylistFilepath];
        
            DBHelper::getFileStatsDBmod(detailViewController.mPlaylist[i].mPlaylistFilename,
                                        detailViewController.mPlaylist[i].mPlaylistFilepath,
                                        &(playlist->entries[i].playcounts),
                                        &(detailViewController.mPlaylist[i].mPlaylistRating),
                                        &(playlist->entries[i].song_length),
                                        &(playlist->entries[i].channels_nb),
                                        &(playlist->entries[i].songs));
        playlist->entries[i].ratings=detailViewController.mPlaylist[i].mPlaylistRating;
    }
    playlist->nb_entries=detailViewController.mPlaylist_size;
    playlist->playlist_name=nil;
    playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Now playing",@"")];
    playlist->playlist_id=nil;
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
    [super viewWillAppear:animated];
    
}

- (void)checkCreate:(NSString *)filePath {
    NSString *completePath=[NSString stringWithFormat:@"%@/%@",NSHomeDirectory(),filePath];
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
    if (mSearch) return nil;
    if (show_playlist) return nil;
    if ((browse_depth>=2)&&(show_playlist==0)) {
        if (section==0) return nil;
        if (section==1) return @"";
        if ((search_local?search_local_entries_count[section-2]:local_entries_count[section-2])) return [indexTitlesDownload objectAtIndex:section];
        return nil;
    }
    if (browse_depth>=2) return [indexTitles objectAtIndex:section];
    return nil;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    local_flag=0;
    if (browse_depth==0) return [keys count];
    if (show_playlist) return 1;
    if ((show_playlist==0)&&(browse_depth>=2)) return 28+1;
    return 28;
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
    if (section==0) return 0;
    if (section==1) return 1;
    return (search_local?search_local_entries_count[section-2]:local_entries_count[section-2]);
}
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    if (mSearch) return nil;
    if (show_playlist) return nil;
    if ((browse_depth>=2)&&(show_playlist==0)) return indexTitlesDownload;
    if (browse_depth>=2) return indexTitles;
    return nil;
}

- (NSInteger)tableView:(UITableView *)tabView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    if (mSearch) return -1;
    if (show_playlist) return -1;
    if (index == 0) {
        [tabView setContentOffset:CGPointZero animated:NO];
        return NSNotFound;
    }
    return index;
    
}

/**
 Asks the delegate if the cell can be a slide-state.
 
 The result of this function is not reflected to the slide indicators of the cell.
 You should set "showsLeftSlideIndicator" or "showsRightSlideIndicator" property of SESlideTableViewCell manually.
 
 @return YES if the cell can be the state, otherwise NO.
 @param cell The cell that is making this request.
 @param slideState The state that the cell want to be.
 */
- (BOOL)slideTableViewCell:(SESlideTableViewCell*)cell canSlideToState:(SESlideTableViewCellSlideState)slideState {
    return YES;
}


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
                            signed char rating;

                            DBHelper::getFileStatsDBmod(playlist->entries[indexPath.row-1].label,
                                                        playlist->entries[indexPath.row-1].fullpath,
                                                        &playcount,&rating);
                            playcount=0;
                            DBHelper::updateFileStatsDBmod(playlist->entries[indexPath.row-1].label,
                                                           playlist->entries[indexPath.row-1].fullpath,
                                                           playcount,rating);
                    
                            [self loadMostPlayedList];

                            forceReloadCells=true;
                            [self.tableView reloadData];
                            //[self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
                    
                } else if (integrated_playlist==INTEGRATED_PLAYLIST_FAVORITES) {
                            short int playcount;
                            signed char rating;
                            DBHelper::getFileStatsDBmod(playlist->entries[indexPath.row-1].label,
                                                        playlist->entries[indexPath.row-1].fullpath,
                                                        &playcount,&rating);
                            rating=0;
                            DBHelper::updateFileStatsDBmod(playlist->entries[indexPath.row-1].label,
                                                           playlist->entries[indexPath.row-1].fullpath,
                                                           playcount,rating);
                    
                            [self loadFavoritesList];
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

- (void)slideTableViewCell:(SESlideTableViewCell*)cell didTriggerLeftButton:(NSInteger)buttonIndex {
}

- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath {    
    static NSString *CellIdentifier = @"Cell";
    static NSString *CellIdentifier_withAction = @"CellA";
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
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    BOOL isEditing=[tabView isEditing];
    
    SESlideTableViewCell *cell;
    
    bool allow_delete;
    allow_delete=false;
    if (browse_depth==0) {
        //main playlist screen with builtin ones + user specific
        if (indexPath.row>=5) allow_delete=true;
    } else if (browse_depth==1) {
        if (show_playlist) {
            if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) {
                if (indexPath.row>=1) allow_delete=true;
            } else if (integrated_playlist==INTEGRATED_PLAYLIST_RANDOM) {
                if (indexPath.row>=1) allow_delete=false;
            } else if (integrated_playlist==INTEGRATED_PLAYLIST_MOSTPLAYED) {
                if (indexPath.row>=1) allow_delete=true;
            } else if (integrated_playlist==INTEGRATED_PLAYLIST_FAVORITES) {
                if (indexPath.row>=1) allow_delete=true;
            } else if (integrated_playlist==0) {
                if (indexPath.row>=2) allow_delete=true;
            }
        }
    }
    
    if (allow_delete) cell = (SESlideTableViewCell *)[tabView dequeueReusableCellWithIdentifier:CellIdentifier_withAction];
    else cell = (SESlideTableViewCell *)[tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    if ((cell == nil)||forceReloadCells) {
        if (!allow_delete) {
            cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
            cell.delegate=self;
        } else {
            cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier_withAction];
            cell.delegate=self;
            [cell addRightButtonWithText:NSLocalizedString(@"Delete",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor redColor]];
        }
        
        cell.frame=CGRectMake(0,0,tabView.frame.size.width,40);
        
        [cell setBackgroundColor:[UIColor clearColor]];        
        
        NSString *imgFile=(darkMode?@"tabview_gradient40Black.png":@"tabview_gradient40.png");
        UIImage *image = [UIImage imageNamed:imgFile];
        
        MDZUIImageView *imageView = [[MDZUIImageView alloc] initWithImage:image];
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
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
        bottomImageView = (UIImageView *)[cell viewWithTag:BOTTOM_IMAGE_TAG];
        actionView = (UIButton *)[cell viewWithTag:ACT_IMAGE_TAG];
        secActionView = (UIButton *)[cell viewWithTag:SECACT_IMAGE_TAG];
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
                               tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-(isEditing?32:0),
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
                                
                if ((playlist->entries[row-2].ratings==-1)||(playlist->entries[row-2].playcounts==-1)) {
                    DBHelper::getFileStatsDBmod(playlist->entries[row-2].label,
                                                playlist->entries[row-2].fullpath,
                                                &(playlist->entries[row-2].playcounts),
                                                &(playlist->entries[row-2].ratings),
                                                &(playlist->entries[row-2].song_length),
                                                &(playlist->entries[row-2].channels_nb),
                                                &(playlist->entries[row-2].songs));
                    if (playlist->playlist_id==nil) {//current queue
                        detailViewController.mPlaylist[row-2].mPlaylistRating=playlist->entries[row-2].ratings;
                    }
                }

                if (playlist->entries[row-2].ratings>0) bottomImageView.image=[UIImage imageNamed:ratingImg[RATING_IMG(playlist->entries[row-2].ratings)]];
                NSArray *filename_parts=[playlist->entries[row-2].fullpath componentsSeparatedByString:@"/"];
                
                NSString *tmp_str;
                
                if ([filename_parts count]>=3) {
                    if ([(NSString*)[filename_parts objectAtIndex:[filename_parts count]-3] compare:@"Documents"]!=NSOrderedSame) {
                        tmp_str = [NSString stringWithFormat:@"%@/%@|",[filename_parts objectAtIndex:[filename_parts count]-3],[filename_parts objectAtIndex:[filename_parts count]-2]];
                    } else tmp_str = [NSString stringWithFormat:@"%@|",[filename_parts objectAtIndex:[filename_parts count]-2]];
                } else if ([filename_parts count]>=2) {
                    if ([(NSString*)[filename_parts objectAtIndex:[filename_parts count]-2] compare:@"Documents"]!=NSOrderedSame) {
                        tmp_str = [NSString stringWithFormat:@"%@|",[filename_parts objectAtIndex:[filename_parts count]-2]];
                    } else tmp_str = @"";
                }
                
                bottomLabel.text=[tmp_str stringByAppendingFormat:@"Pl:%d",playlist->entries[row-2].playcounts];
                
                
                bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+20,
                                               22,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth-32-20,
                                               18);
                
            }
        } else {
            if (indexPath.section==1) {
                cellValue=(mShowSubdir?NSLocalizedString(@"DisplayDir_MainKey",""):NSLocalizedString(@"DisplayAll_MainKey",""));
                if (darkMode) topLabel.textColor=[UIColor colorWithRed:0.4f green:0.4f blue:0.9f alpha:1.0];
                else topLabel.textColor=[UIColor colorWithRed:0.4f green:0.4f blue:0.9f alpha:1.0];
                bottomLabel.text=(mShowSubdir?NSLocalizedString(@"DisplayDir_SubKey",""):NSLocalizedString(@"DisplayAll_SubKey",""));
                bottomLabel.text=[NSString stringWithFormat:@"%@ %d files",(mShowSubdir?NSLocalizedString(@"DisplayDir_SubKey",""):NSLocalizedString(@"DisplayAll_SubKey","")),(search_local?search_local_nb_entries:local_nb_entries)];
                
                
                
                
                bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                               22,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                               18);
                
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-4-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                           22);
                
                [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateNormal];
                [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateHighlighted];
                [secActionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                
                [actionView setImage:[UIImage imageNamed:@"playlist_del_all.png"] forState:UIControlStateNormal];
                [actionView setImage:[UIImage imageNamed:@"playlist_del_all.png"] forState:UIControlStateHighlighted];
                [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                
                actionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                actionView.enabled=YES;
                actionView.hidden=NO;
                secActionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE-PRI_SEC_ACTIONS_IMAGE_SIZE-4-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                secActionView.enabled=YES;
                secActionView.hidden=NO;
                
            } else {
                
                if (cur_local_entries[indexPath.section-2][indexPath.row].type==0) { //directory
                    cellValue=cur_local_entries[indexPath.section-2][indexPath.row].label;
                    if (darkMode) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:1.0f alpha:1.0f];
                    else topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                               0,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                                               34);
                    
                } else  { //file
                    int nb_occur;
                    NSString *tmp_str;
                    cellValue=cur_local_entries[indexPath.section-2][indexPath.row].label;
                    cell.accessoryType = UITableViewCellAccessoryNone;
                    
                    int actionicon_offsetx=tabView.safeAreaInsets.left+tabView.safeAreaInsets.right;
                    //archive file ?
                    if ((cur_local_entries[indexPath.section-2][indexPath.row].type==2)||(cur_local_entries[indexPath.section-2][indexPath.row].type==3)) {
                        actionicon_offsetx=PRI_SEC_ACTIONS_IMAGE_SIZE+tabView.safeAreaInsets.left+tabView.safeAreaInsets.right;
                        //                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                        secActionView.frame = CGRectMake(tabView.bounds.size.width-2-32-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                        [secActionView setImage:[UIImage imageNamed:@"arc_details.png"] forState:UIControlStateNormal];
                        [secActionView setImage:[UIImage imageNamed:@"arc_details.png"] forState:UIControlStateHighlighted];
                        [secActionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                        [secActionView addTarget: self action: @selector(accessoryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                        secActionView.enabled=YES;
                        secActionView.hidden=NO;
                    }
                    
                    
                    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                               0,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,
                                               22);
                    
                    
                    if (cur_local_entries[indexPath.section-2][indexPath.row].rating==-1) {
                        DBHelper::getFileStatsDBmod(
                                                    cur_local_entries[indexPath.section-2][indexPath.row].label,
                                                    cur_local_entries[indexPath.section-2][indexPath.row].fullpath,
                                                    &cur_local_entries[indexPath.section-2][indexPath.row].playcount,
                                                    &cur_local_entries[indexPath.section-2][indexPath.row].rating);
                    }
                    if (cur_local_entries[indexPath.section-2][indexPath.row].rating>0) bottomImageView.image=[UIImage imageNamed:ratingImg[RATING_IMG(cur_local_entries[indexPath.section-2][indexPath.row].rating)]];
                    tmp_str = [NSString stringWithFormat:@"Pl:%d",cur_local_entries[indexPath.section-2][indexPath.row].playcount];
                    
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+20,
                                                   22,
                                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-20-actionicon_offsetx,
                                                   18);
                    if ((nb_occur=[self isLocalEntryInPlaylist:cur_local_entries[indexPath.section-2][indexPath.row].fullpath])) {
                        
                        [actionView setImage:[UIImage imageNamed:@"playlist_del.png"] forState:UIControlStateNormal];
                        [actionView setImage:[UIImage imageNamed:@"playlist_del.png"] forState:UIControlStateHighlighted];
                        [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                        [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                        actionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
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
                signed char rating;
                DBHelper::getFileStatsDBmod(playlist->entries[indexPath.row-rowofs].label,
                                            playlist->entries[indexPath.row-rowofs].fullpath,
                                            &playcount,&rating);
                playcount=0;
                DBHelper::updateFileStatsDBmod(playlist->entries[indexPath.row-rowofs].label,
                                               playlist->entries[indexPath.row-rowofs].fullpath,
                                               playcount,rating);
                [self loadMostPlayedList];
                [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
            } else if (integrated_playlist==INTEGRATED_PLAYLIST_FAVORITES) {  //favorites: reset rating
                short int playcount;
                signed char rating;
                DBHelper::getFileStatsDBmod(playlist->entries[indexPath.row-rowofs].label,
                                            playlist->entries[indexPath.row-rowofs].fullpath,
                                            &playcount,&rating);
                rating=0;
                DBHelper::updateFileStatsDBmod(playlist->entries[indexPath.row-rowofs].label,
                                               playlist->entries[indexPath.row-rowofs].fullpath,
                                               playcount,rating);
                [self loadFavoritesList];
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
        /*if (integrated_playlist==INTEGRATED_PLAYLIST_NOWPLAYING) {
            if (indexPath.row-rowofs==detailViewController.mPlaylist_pos) return NO;
        }*/
        return YES;
    }
    return NO;
}
- (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    //NSLog(@"depth %d / integrated pl %d / show_pl %d | sect %d row %d",browse_depth,integrated_playlist,show_playlist,indexPath.section,indexPath.row);
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
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    shouldFillKeys=1;
    search_local=0;
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
        
        
        
    } else {
        [self showAlertMsg:@"Warning" message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"")];
    }
}

#pragma mark -
#pragma mark Table view delegate
- (void) primaryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    
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
                [self loadMostPlayedList];
                playlist->playlist_name=[[NSString alloc] initWithFormat:@"Most played"];
                playlist->playlist_id=nil;
            } else if (indexPath.row==4) {
                [self loadFavoritesList];
                playlist->playlist_name=[[NSString alloc] initWithFormat:@"Favorites"];
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
            if (indexPath.section==1) {
                //remove all
                for (int i=0;i<27;i++)
                    for (int j=0;j<(search_local?search_local_entries_count[i]:local_entries_count[i]);j++)
                        if (cur_local_entries[i][j].type&3) {
                            int found=-1;
                            for (int ii=0;ii<playlist->nb_entries;ii++) {
                                if ([playlist->entries[ii].fullpath compare:cur_local_entries[i][j].fullpath]==NSOrderedSame) found=ii;
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
                    if ([playlist->entries[i].fullpath compare:cur_local_entries[indexPath.section-2][indexPath.row].fullpath]==NSOrderedSame) found=i;
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
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    
    int section=indexPath.section-1;
    if (show_playlist) {
    } else { //browsing for playlist, add selected file to playlist
        if (indexPath.section==1) {
            //add all
            for (int i=0;i<27;i++) {
                for (int j=0;j<(search_local?search_local_entries_count[i]:local_entries_count[i]);j++) {
                    if (cur_local_entries[i][j].type&3) {
                        if (playlist->nb_entries<MAX_PL_ENTRIES) {
                            playlist->nb_entries++;
                            
                            playlist->entries[playlist->nb_entries-1].label=[[NSString alloc] initWithFormat:@"%@",cur_local_entries[i][j].label];
                            playlist->entries[playlist->nb_entries-1].fullpath=[[NSString alloc] initWithFormat:@"%@",cur_local_entries[i][j].fullpath];
                            
                            playlist->entries[playlist->nb_entries-1].ratings=cur_local_entries[i][j].rating;
                            playlist->entries[playlist->nb_entries-1].playcounts=cur_local_entries[i][j].playcount;
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
                if (playlist->nb_entries>=MAX_PL_ENTRIES) break;
            }
            [tableView reloadData];
            
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
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    
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
            for (int i=0;i<detailViewController.mPlaylist_size;i++) {
                playlist->entries[i].label=[[NSString alloc] initWithString:detailViewController.mPlaylist[i].mPlaylistFilename];
                playlist->entries[i].fullpath=[[NSString alloc ] initWithString:detailViewController.mPlaylist[i].mPlaylistFilepath];
                
                DBHelper::getFileStatsDBmod(detailViewController.mPlaylist[i].mPlaylistFilename,
                                            detailViewController.mPlaylist[i].mPlaylistFilepath,
                                            &(playlist->entries[i].playcounts),
                                            &(detailViewController.mPlaylist[i].mPlaylistRating),
                                            &(playlist->entries[i].song_length),
                                            &(playlist->entries[i].channels_nb),
                                            &(playlist->entries[i].songs));
                playlist->entries[i].ratings=detailViewController.mPlaylist[i].mPlaylistRating;
            }
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
            childController.view.frame=self.view.frame;
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
            childController.view.frame=self.view.frame;
            keys=nil;
            list=nil;
            
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
        }
        if (indexPath.row==3) { //most played
            [self loadMostPlayedList];
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
            childController.view.frame=self.view.frame;
            keys=nil;
            list=nil;
            
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
        }
        if (indexPath.row==4) { //favorites
            [self loadFavoritesList];
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
            childController.view.frame=self.view.frame;
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
            childController.view.frame=self.view.frame;
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
                childController.view.frame=self.view.frame;
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
                            [self sortAZPlaylist];
                        }];
                    [alertC addAction:sortAZAction];
                    
                    UIAlertAction* sortZAAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Sort Z->A",@"") style:UIAlertActionStyleDefault
                        handler:^(UIAlertAction * action) {
                            [self sortZAPlaylist];
                        }];
                    [alertC addAction:sortZAAction];
                    
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
                                [self sortAZPlaylist];
                            }];
                        [alertC addAction:sortAZAction];
                        
                        UIAlertAction* sortZAAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Sort Z->A",@"") style:UIAlertActionStyleDefault
                            handler:^(UIAlertAction * action) {
                                [self sortZAPlaylist];
                            }];
                        [alertC addAction:sortZAAction];
                                                
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
            if (indexPath.section==1) {
                mShowSubdir^=1;
                shouldFillKeys=1;
                
                [self showWaiting];
                [self flushMainLoop];
                
                [self fillKeys];
                [tabView reloadData];
                [self.tableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionMiddle];
                
                [self hideWaiting];
            } else {
                int section=indexPath.section-2;
                cellValue=cur_local_entries[section][indexPath.row].label;
                
                if (cur_local_entries[section][indexPath.row].type==0) { //Directory selected : change current directory
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
                    childController.view.frame=self.view.frame;
                    // And push the window
                    [self.navigationController pushViewController:childController animated:YES];
                    
                    //				[childController autorelease];
                } else if (((cur_local_entries[section][indexPath.row].type==2)||(cur_local_entries[section][indexPath.row].type==3))&&(mAccessoryButton)) { //Archive selected or multisongs: display files inside
                    
                    [self showWaiting];
                    [self flushMainLoop];
                    
                    NSString *newPath;
                    //                    NSLog(@"currentPath:%@\ncellValue:%@\nfullpath:%@",currentPath,cellValue,cur_local_entries[section][indexPath.row].fullpath);
                    if (mShowSubdir) newPath=[NSString stringWithString:cur_local_entries[section][indexPath.row].fullpath];
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
                    childController.view.frame=self.view.frame;
                    // And push the window
                    [self.navigationController pushViewController:childController animated:YES];
                    
                    
                    [self hideWaiting];
                    //				[childController autorelease];
                } else {  //File selected : add to playlist
                    if (playlist->nb_entries<MAX_PL_ENTRIES) {
                        playlist->nb_entries++;
                        playlist->entries[playlist->nb_entries-1].label=[[NSString alloc] initWithFormat:@"%@",cur_local_entries[section][indexPath.row].label];
                        playlist->entries[playlist->nb_entries-1].fullpath=[[NSString alloc] initWithFormat:@"%@",cur_local_entries[section][indexPath.row].fullpath];
                        
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
- (void)freePlaylist {
    if (!mFreePlaylist) return;
    
    NSLog(@"free  playlist");
    
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
    
    waitingView=nil;
    
    mSearchText=nil;
        
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {
            local_entries_data[i].label=nil;
            local_entries_data[i].fullpath=nil;
        }
        for (int i=0;i<27;i++) {
            for (int j=0;j<local_entries_count[i];j++) {
                local_entries[i][j].label=nil;
                local_entries[i][j].fullpath=nil;
            }
            local_entries[i]=NULL;
        }
        free(local_entries_data);local_entries_data=NULL;
        local_nb_entries=0;
    }
    for (int i=0;i<27;i++) local_entries_count[i]=0;
    
    if (search_local_nb_entries) {
        for (int i=0;i<27;i++) {
            for (int j=0;j<search_local_entries_count[i];j++) {
                search_local_entries[i][j].label=nil;
                search_local_entries[i][j].fullpath=nil;
            }
            search_local_entries[i]=NULL;
        }
        search_local_nb_entries=0;
        free(search_local_entries_data);
    }
    
    indexTitles=nil;
    indexTitlesDownload=nil;
    mFileMngr=nil;
    if (mFreePlaylist) [self freePlaylist];
    keys=nil;
    list=nil;
    //[super dealloc];
}

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
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


@end
