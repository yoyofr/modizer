//
//  RootViewControllerSMSPWebParser.mm
//  modizer1
//
//  Created by Yohann Magnien on 07/05/21.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//


#define RATING_IMG(a) ( (a==5?2:(a?1:0)) )

#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define ROW_HEIGHT 40

#include <sys/types.h>
#include <sys/sysctl.h>

#include <pthread.h>
#include <stdlib.h>
static volatile int mPopupAnimation=0;

#import "AppDelegate_Phone.h"
#import "RootViewControllerSMSPWebParser.h"
#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
#import "WebBrowser.h"
#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];

#import "TFHpple.h"

#import "CBAutoScrollLabel.h"

#import "QuartzCore/CAAnimation.h"
#import "TTFadeAnimator.h"

#import "AFNetworking.h"
#import "AFHTTPSessionManager.h"
#import "AFURLSessionManager.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;

enum {
    BROWSE_ALL=0,
    BROWSE_SYSTEMS
};


@implementation RootViewControllerSMSPWebParser

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

int qsortSMSP_entries_alpha(const void *entryA, const void *entryB) {
    NSString *strA,*strB;
    NSComparisonResult res;
    strA=((t_WEB_browse_entry*)entryA)->label;
    strB=((t_WEB_browse_entry*)entryB)->label;
    res=[strA localizedCaseInsensitiveCompare:strB];
    if (res==NSOrderedAscending) return -1;
    if (res==NSOrderedSame) return 0;
    return 1; //NSOrderedDescending
}

int qsortSMSP_entries_rating_or_entries(const void *entryA, const void *entryB) {
    if (((t_WEB_browse_entry*)entryA)->isFile) {
        float rA,rB;
        rA=((t_WEB_browse_entry*)entryA)->webRating;
        rB=((t_WEB_browse_entry*)entryB)->webRating;
        if (rA>rB) return -1;
        if (rA<rB) return 1;
        //if same, use label
        NSString *strA,*strB;
        NSComparisonResult res;
        strA=((t_WEB_browse_entry*)entryA)->label;
        strB=((t_WEB_browse_entry*)entryB)->label;
        res=[strA localizedCaseInsensitiveCompare:strB];
        if (res==NSOrderedAscending) return -1;
        if (res==NSOrderedSame) return 0;
        return 1; //NSOrderedDescending
    } else {
        int sA,sB;
        sA=((t_WEB_browse_entry*)entryA)->entries_nb;
        sB=((t_WEB_browse_entry*)entryB)->entries_nb;
        if (sA>sB) return -1;
        if (sA<sB) return 1;
        //if same, use label
        NSString *strA,*strB;
        NSComparisonResult res;
        strA=((t_WEB_browse_entry*)entryA)->label;
        strB=((t_WEB_browse_entry*)entryB)->label;
        res=[strA localizedCaseInsensitiveCompare:strB];
        if (res==NSOrderedAscending) return -1;
        if (res==NSOrderedSame) return 0;
        return 1; //NSOrderedDescending
    }
    
    return 0;
}

-(void) titleTap {
    if (!dbWEB_nb_entries) return;
    if (sort_mode==0) {
        if (dbWEB_entries_data->isFile) sort_mode=1;
        else sort_mode=2;
    } else sort_mode=0;
    
    if (sort_mode==0) {
        navbarTitle.text=[NSString stringWithFormat:@"%@ (name)",self.title];
        self.navigationItem.title=navbarTitle.text;
        qsort(dbWEB_entries_data,dbWEB_nb_entries,sizeof(t_WEB_browse_entry),qsortSMSP_entries_alpha);
        [self fillKeys]; //update search if active
    } else {
        if (sort_mode==1) {
            navbarTitle.text=[NSString stringWithFormat:@"%@ (rating)",self.title];
            self.navigationItem.title=navbarTitle.text;
        } else {
            navbarTitle.text=[NSString stringWithFormat:@"%@ (packs)",self.title];
            self.navigationItem.title=navbarTitle.text;
        }
        qsort(dbWEB_entries_data,dbWEB_nb_entries,sizeof(t_WEB_browse_entry),qsortSMSP_entries_rating_or_entries);
        [self fillKeys]; //update search if active
    }
    
    [tableView reloadData];
}

- (void)viewDidLoad {
    clock_t start_time,end_time;
    start_time=clock();
    childController=NULL;
    
    indexTitleMode=0;
    
    sort_mode=0;
    //set default sort mode
    if ([mWebBaseURL isEqualToString:@"https://project2612.org/list.php?sort=rating"]) sort_mode=1;
    
    
    browse_mode=BROWSE_ALL;
    if ([self.title isEqualToString:@"Systems"]) {
        browse_mode=BROWSE_SYSTEMS;
    }
    
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
    
    imagesCache = [[ImagesCache alloc] init];
        
    navbarTitle=[[UILabel alloc] init];
    navbarTitle.userInteractionEnabled=TRUE;
    
    if (browse_depth>0) {
        self.navigationItem.titleView=navbarTitle;
        
        if (sort_mode==0) {
            navbarTitle.text=[NSString stringWithFormat:@"%@ (name)   ",self.title];
            self.navigationItem.title=navbarTitle.text;
        } else {
            if (sort_mode==1) {
                navbarTitle.text=[NSString stringWithFormat:@"%@ (rating)",self.title];
                self.navigationItem.title=navbarTitle.text;
            } else {
                navbarTitle.text=[NSString stringWithFormat:@"%@ (packs) ",self.title];
                self.navigationItem.title=navbarTitle.text;
            }
        }
        
        UITapGestureRecognizer *tapGesture =
        [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(titleTap)];
        [navbarTitle addGestureRecognizer:tapGesture];
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
    if (shouldFillKeys) {
        shouldFillKeys=0;
        if (browse_depth==0) [self fillKeysWithRepoCateg];
        else [self fillKeysWithWEBSource];
       
    } else { //reset downloaded, rating & playcount flags
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].downloaded=-1;
            dbWEB_entries_data[i].rating=-1;
            dbWEB_entries_data[i].playcount=-1;
        }
        if (mSearch) {
            if (browse_depth==0) [self fillKeysWithRepoCateg];            
            else [self fillKeysWithWEBSource];
        }
    }
}

-(void) fillKeysWithRepoCateg {
    int dbWEB_entries_index;
    int index,previndex;
    
    NSRange r;
    
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
    dbWEB_hasFiles=search_dbWEB_hasFiles=0;
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    if (mSearch) {
        search_dbWEB=1;
        
        search_dbWEB_entries_data=(t_WEB_browse_entry*)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
        
        for (int i=0;i<(indexTitleMode?27:1);i++) {
            search_dbWEB_entries_count[i]=0;
            if (dbWEB_entries_count[i]) search_dbWEB_entries[i]=&(search_dbWEB_entries_data[search_dbWEB_nb_entries]);
            for (int j=0;j<dbWEB_entries_count[i];j++)  {
                r.location=NSNotFound;
                r = [dbWEB_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].label=dbWEB_entries[i][j].label;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].downloaded=dbWEB_entries[i][j].downloaded;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].rating=dbWEB_entries[i][j].rating;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].playcount=dbWEB_entries[i][j].playcount;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].fullpath=dbWEB_entries[i][j].fullpath;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].URL=dbWEB_entries[i][j].URL;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].isFile=dbWEB_entries[i][j].isFile;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].info=dbWEB_entries[i][j].info;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].img_URL=dbWEB_entries[i][j].img_URL;
                    search_dbWEB_entries_count[i]++;
                    search_dbWEB_nb_entries++;
                }
            }
        }
        return;
    }
    if (dbWEB_nb_entries) {
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].label=nil;
            dbWEB_entries_data[i].fullpath=nil;
            dbWEB_entries_data[i].URL=nil;
            dbWEB_entries_data[i].info=nil;
            dbWEB_entries_data[i].img_URL=nil;
        }
        free(dbWEB_entries_data);dbWEB_entries_data=NULL;
        dbWEB_nb_entries=0;
    }
    
    typedef struct {
        NSString *category;
        NSString *url;
    } t_categ_entry;
    NSArray *sortedArray;
    NSMutableArray *tmpArray=[[NSMutableArray alloc] init];
    t_categ_entry webs_entry[]= {
        {@"All",@"https://www.smspower.org/Music/VGMs"},
        {@"Systems",@"https://www.smspower.org/Music/VGMs"}
    };

    for (int i=0;i<sizeof(webs_entry)/sizeof(t_categ_entry);i++) [tmpArray addObject:[NSValue valueWithPointer:&webs_entry[i]]];
    
    if (indexTitleMode) {
        sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
            NSString *str1=[((t_categ_entry*)[obj1 pointerValue])->category  lastPathComponent];
            NSString *str2=[((t_categ_entry*)[obj2 pointerValue])->category lastPathComponent];
            return [str1 caseInsensitiveCompare:str2];
        }];
    } else {
        sortedArray=tmpArray;
    }

    
    ////

    dbWEB_nb_entries=[sortedArray count];
    
    //2nd initialize array to receive entries
    dbWEB_entries_data=(t_WEB_browse_entry *)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    memset(dbWEB_entries_data,0,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    dbWEB_entries_index=0;
    for (int i=0;i<(indexTitleMode?27:1);i++) {
        dbWEB_entries_count[i]=0;
        dbWEB_entries[i]=NULL;
    }
    
    char chr;
    index=-1;
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_categ_entry *wentry = (t_categ_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        //sprintf(str,"%s",[wentry->category UTF8String]);
        chr=[wentry->category characterAtIndex:0];
        previndex=index;
        index=0;
        if (indexTitleMode) {
            if ((chr>='A')&&(chr<='Z') ) index=(chr-'A'+1);
            if ((chr>='a')&&(chr<='z') ) index=(chr-'a'+1);
        }
        //sections are determined 'on the fly' since result set is already sorted
        if (previndex!=index) {
            if (previndex>index) {
                //NSLog(@"********* %s",str);
            } else dbWEB_entries[index]=&(dbWEB_entries_data[dbWEB_entries_index]);
        }
        
        dbWEB_entries[index][dbWEB_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%@",wentry->category];
                
        dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@",wentry->category];
                
        dbWEB_entries[index][dbWEB_entries_count[index]].URL=[NSString stringWithString:wentry->url];
                                
        dbWEB_entries[index][dbWEB_entries_count[index]].isFile=0;
                
        dbWEB_entries_count[index]++;
        dbWEB_entries_index++;
    }
    //populate entries
}

-(void) fillKeysWithWEBSource {
    int dbWEB_entries_index;
    int index,previndex;
    
    NSRange r;
    
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
    dbWEB_hasFiles=search_dbWEB_hasFiles=0;
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    if (mSearch) {
        search_dbWEB=1;
        
        search_dbWEB_entries_data=(t_WEB_browse_entry*)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
        
        for (int i=0;i<(indexTitleMode?27:1);i++) {
            search_dbWEB_entries_count[i]=0;
            if (dbWEB_entries_count[i]) search_dbWEB_entries[i]=&(search_dbWEB_entries_data[search_dbWEB_nb_entries]);
            for (int j=0;j<dbWEB_entries_count[i];j++)  {
                r.location=NSNotFound;
                r = [dbWEB_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].label=dbWEB_entries[i][j].label;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].downloaded=dbWEB_entries[i][j].downloaded;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].rating=dbWEB_entries[i][j].rating;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].playcount=dbWEB_entries[i][j].playcount;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].fullpath=dbWEB_entries[i][j].fullpath;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].URL=dbWEB_entries[i][j].URL;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].isFile=dbWEB_entries[i][j].isFile;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].info=dbWEB_entries[i][j].info;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].img_URL=dbWEB_entries[i][j].img_URL;
                    search_dbWEB_entries_count[i]++;
                    search_dbWEB_nb_entries++;
                }
            }
        }
        return;
    }
    if (dbWEB_nb_entries) {
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].label=nil;
            dbWEB_entries_data[i].fullpath=nil;
            dbWEB_entries_data[i].URL=nil;
            dbWEB_entries_data[i].info=nil;
            dbWEB_entries_data[i].img_URL=nil;
        }
        free(dbWEB_entries_data);dbWEB_entries_data=NULL;
        dbWEB_nb_entries=0;
    }
    
    typedef struct {
        NSString *file_URL;
        NSString *file_name;
        NSString *file_company;
        NSString *file_systems;
        NSString *file_chipsets;
        float file_rating;
        int entries_nb;
        NSString *file_details;
        NSString *file_img_URL;
        char file_type;
    } t_web_file_entry;
    
    //Browse page
    //Download html data
    NSURL *url;
    NSData  *urlData;
    TFHpple * doc;
    NSArray *sortedArray;
    NSMutableArray *tmpArray=[[NSMutableArray alloc] init];
    t_web_file_entry *we=NULL;
          
    if (browse_mode==BROWSE_SYSTEMS) {
        ///////////////////////////////////////////////////////////////////////:
        // SMSP All
        ///////////////////////////////////////////////////////////////////////:
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
            
        NSArray *arr_url_dirty=[doc searchWithXPathQuery:@"/html/body//tr/td/a[contains(@href,'field=composer') and not(contains(@href,'%E'))]"];
        NSMutableArray *arr_url=[[NSMutableArray alloc] init];
        NSMutableArray *arr_name=[[NSMutableArray alloc] init];
        NSMutableDictionary *dic_entries=[[NSMutableDictionary alloc] init];
        for (int i=0;i<[arr_url_dirty count];i++) {
            TFHppleElement *el=[arr_url_dirty objectAtIndex:i];
            NSString *url=[el objectForKey:@"href"];
            //NSLog(@"url: %@",url);
            NSString *name=[el text];
            //NSLog(@"name: %@",name);
            if (name==nil) {
                //NSLog(@"yo");
            } else {
                if (![arr_name containsObject:name]) {
                    [arr_name addObject:name];
                    [arr_url addObject:url];
                    [dic_entries setValue:@1 forKey:name];
                } else {
                    int nb_entries=[[dic_entries valueForKey:name] intValue];
                    nb_entries++;
                    [dic_entries setValue:[NSNumber numberWithInt:nb_entries] forKey:name];
                }
            }
        }
        
        /*NSArray *arr_system=[doc searchWithXPathQuery:@"/html/body//tr/td[@class='c'][1]"];
        NSArray *arr_size=[doc searchWithXPathQuery:@"/html/body//tr/td[@class='c'][2]"];
        NSArray *arr_rating=[doc searchWithXPathQuery:@"/html/body//tr/td[@class='c'][3]"];*/
        
        we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[arr_url count]);
        int we_index=0;
                
        if (arr_url&&[arr_url count]) {
            
            for (int j=0;j<[arr_url count];j++) {
                we[we_index].file_URL=[NSString stringWithFormat:@"https://project2612.org/%@",[arr_url objectAtIndex:j]];
                                                          
                //el=[arr_url objectAtIndex:j];
                we[we_index].file_name=[NSString stringWithString:[arr_name objectAtIndex:j]];
                
                we[we_index].file_type=0;
                
                we[we_index].entries_nb=[[dic_entries valueForKey:we[we_index].file_name] intValue];
                
                [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                
                we_index++;
            }
        }
    } else if (browse_mode==BROWSE_ALL) {
        ///////////////////////////////////////////////////////////////////////:
        // SMSP All
        ///////////////////////////////////////////////////////////////////////:
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
            
        NSArray *arr_url=[doc searchWithXPathQuery:@"/html/body//h3//a[@class='wikilink']"];
        NSArray *arr_system=[doc searchWithXPathQuery:@"/html/body//tr[position()>1]/td[2]/p"];
        NSArray *arr_filesize=[doc searchWithXPathQuery:@"/html/body//tr[position()>1]/td[3]/div[1]/text()"];
        NSArray *arr_fileinfo=[doc searchWithXPathQuery:@"/html/body//tr[position()>1]/td[3]/div[2]/p"];
        
        we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[arr_url count]);
        int we_index=0;
                
        if (arr_url&&[arr_url count]) {
            
            for (int j=0;j<[arr_url count];j++) {
                TFHppleElement *el=[arr_url objectAtIndex:j];
                
                we[we_index].file_URL=[[[NSString stringWithFormat:@"http:%@",[el objectForKey:@"href"]] stringByAppendingString:@".zip"] stringByReplacingOccurrencesOfString:@"/Music" withString:@"/uploads/Music"];
                                                          
                we[we_index].file_img_URL=[[[[NSString stringWithFormat:@"http:%@",[el objectForKey:@"href"]] stringByDeletingPathExtension] stringByAppendingString:@".png"] stringByReplacingOccurrencesOfString:@"/Music" withString:@"/uploads/Music"];
                
                we[we_index].file_name=[[[[el text] stringByReplacingOccurrencesOfString:@"/" withString:@"-"]  stringByReplacingOccurrencesOfString:@"?" withString:@""] stringByReplacingOccurrencesOfString:@"\n" withString:@""];
                we[we_index].file_type=1;
                
                we[we_index].file_rating=0;
                
                el=[arr_system objectAtIndex:j];
                we[we_index].file_systems=[[[NSString stringWithString:[el text]] stringByReplacingOccurrencesOfString:@"\n" withString:@""] stringByReplacingOccurrencesOfString:@"/" withString:@","];
                
                el=[arr_filesize objectAtIndex:j];
                we[we_index].file_details=[NSString stringWithFormat:@"%@",[el raw]];
                
                el=[arr_fileinfo objectAtIndex:j];
                we[we_index].file_details=[NSString stringWithFormat:@"%@・%@",we[we_index].file_details,[el text]];
                
                [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                we_index++;
            }
        }
    }
        
    if (indexTitleMode) {
        sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
            NSString *str1=[((t_web_file_entry*)[obj1 pointerValue])->file_name lastPathComponent];
            NSString *str2=[((t_web_file_entry*)[obj2 pointerValue])->file_name lastPathComponent];
            return [str1 caseInsensitiveCompare:str2];
        }];
    } else {
        if (sort_mode) {
            sortedArray=[tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
                t_web_file_entry *e1=((t_web_file_entry*)[obj1 pointerValue]);
                t_web_file_entry *e2=((t_web_file_entry*)[obj2 pointerValue]);
                if (e1->file_type) {
                    if (e1->file_rating>e2->file_rating) return NSOrderedAscending;
                    if (e1->file_rating<e2->file_rating) return NSOrderedDescending;
                    NSString *str1=[e1->file_name lastPathComponent];
                    NSString *str2=[e2->file_name lastPathComponent];
                    return [str1 caseInsensitiveCompare:str2];
                } else {
                    if (e1->entries_nb>e2->entries_nb) return NSOrderedAscending;
                    if (e1->entries_nb<e2->entries_nb) return NSOrderedDescending;
                    NSString *str1=[e1->file_name lastPathComponent];
                    NSString *str2=[e2->file_name lastPathComponent];
                    return [str1 caseInsensitiveCompare:str2];
                }
            }];
        } else sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
            NSString *str1=[((t_web_file_entry*)[obj1 pointerValue])->file_name lastPathComponent];
            NSString *str2=[((t_web_file_entry*)[obj2 pointerValue])->file_name lastPathComponent];
            return [str1 caseInsensitiveCompare:str2];
        }];
    }

    dbWEB_nb_entries=[sortedArray count];

    //2nd initialize array to receive entries
    dbWEB_entries_data=(t_WEB_browse_entry *)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    memset(dbWEB_entries_data,0,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    dbWEB_entries_index=0;
    for (int i=0;i<(indexTitleMode?27:1);i++) {
        dbWEB_entries_count[i]=0;
        dbWEB_entries[i]=NULL;
    }

    char chr;
    index=-1;
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_web_file_entry *wef = (t_web_file_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        //NSLog(@"%@",wef->file_name);
        //sprintf(str,"%s",[[wef->file_name stringByRemovingPercentEncoding] UTF8String]);
        chr=[wef->file_name characterAtIndex:0];
        
        //NSLog(@"%@",wef->file_size);
        previndex=index;
        index=0;
        if (indexTitleMode) {
            if ((chr>='A')&&(chr<='Z') ) index=(chr-'A'+1);
            if ((chr>='a')&&(chr<='z') ) index=(chr-'a'+1);
        }
        //sections are determined 'on the fly' since result set is already sorted
        if (previndex!=index) {
            if (previndex>index) {
                //NSLog(@"********* %s",str);
                if (previndex>=0) index=previndex;
                else {
                    index=0;
                    dbWEB_entries[index]=&(dbWEB_entries_data[dbWEB_entries_index]);
                }
                
            } else dbWEB_entries[index]=&(dbWEB_entries_data[dbWEB_entries_index]);
        }
        
        if (wef->file_type==1) dbWEB_entries[index][dbWEB_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%@.zip",wef->file_name];
        else dbWEB_entries[index][dbWEB_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%@",wef->file_name];
                
        if (wef->file_type==1) dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[NSString stringWithFormat:@"Documents/SMSP/%@/%@.zip",wef->file_systems,wef->file_name];
        else dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@",wef->file_name];
        
        if (wef->file_URL) dbWEB_entries[index][dbWEB_entries_count[index]].URL=[NSString stringWithString:wef->file_URL];
        
        if (wef->file_img_URL && ([wef->file_img_URL characterAtIndex:[wef->file_img_URL length]-1]!='/') ) dbWEB_entries[index][dbWEB_entries_count[index]].img_URL=[NSString stringWithString:wef->file_img_URL];
                
        dbWEB_entries[index][dbWEB_entries_count[index]].isFile=wef->file_type;
        dbWEB_entries[index][dbWEB_entries_count[index]].downloaded=-1;
        if (wef->file_type) {
            dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"%@・%@",wef->file_systems, [wef->file_details stringByReplacingOccurrencesOfString:@"&#13;\n" withString:@""]];
            dbWEB_entries[index][dbWEB_entries_count[index]].webRating=wef->file_rating;
        }
        else {
            if (wef->entries_nb>1) dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"%d Packs",wef->entries_nb];
            else dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"1 Pack"];
            dbWEB_entries[index][dbWEB_entries_count[index]].entries_nb=wef->entries_nb;
        }
        
        dbWEB_entries[index][dbWEB_entries_count[index]].rating=-1;
        dbWEB_entries[index][dbWEB_entries_count[index]].playcount=-1;
        dbWEB_entries_count[index]++;
        dbWEB_entries_index++;
    }
    
    for (int i=0;i<dbWEB_nb_entries;i++) {
        we[i].file_URL=nil;
        we[i].file_img_URL=nil;
        we[i].file_name=nil;
        we[i].file_details=nil;
        we[i].file_company=nil;
        we[i].file_chipsets=nil;
        we[i].file_systems=nil;
    }
    
    mdz_safe_free(we);    
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
    if (childController) [(RootViewControllerSMSPWebParser*)childController refreshViewAfterDownload];
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

- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"Cell";
    NSString *cellValue;
    const NSInteger TOP_LABEL_TAG = 1001;
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    const NSInteger BOTTOM_IMAGE_TAG = 1003;
    const NSInteger ACT_IMAGE_TAG = 1004;
    const NSInteger SECACT_IMAGE_TAG = 1005;
    const NSInteger COVER_IMAGE_TAG = 1006;
    UILabel *topLabel;
    CBAutoScrollLabel *bottomLabel;
    UIImageView *bottomImageView,*coverImgView;
    UIButton *actionView,*secActionView;
    
    t_WEB_browse_entry **cur_db_entries;
    long section = indexPath.section-1;
                
    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    bool has_mini_img=(cur_db_entries[section][indexPath.row].img_URL?TRUE:FALSE);
            
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
        //bottomLabel = [[UILabel alloc] init];
        bottomLabel=[[CBAutoScrollLabel alloc] init];
        //[bottomLabel setFont:[UIFont systemFontOfSize:12]];
        //if (darkMode) bottomLabel.textColor = [UIColor whiteColor];
        //else bottomLabel.textColor = [UIColor blackColor];
        bottomLabel.labelSpacing = 35; // distance between start and end labels
        bottomLabel.pauseInterval = 3.7; // seconds of pause before scrolling starts again
        bottomLabel.scrollSpeed = 30; // pixels per second
        bottomLabel.textAlignment = NSTextAlignmentLeft; // centers text when no auto-scrolling is applied
        bottomLabel.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
        bottomLabel.userInteractionEnabled=false;
        
        [cell.contentView addSubview:bottomLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        bottomLabel.tag = BOTTOM_LABEL_TAG;
        bottomLabel.backgroundColor = [UIColor clearColor];
        bottomLabel.font = [UIFont systemFontOfSize:12];
        //bottomLabel.font = [UIFont fontWithName:@"courier" size:12];
        //bottomLabel.lineBreakMode=NSLineBreakByTruncatingMiddle;
        bottomLabel.opaque=TRUE;
        
        bottomImageView = [[UIImageView alloc] initWithImage:nil];
        bottomImageView.frame = CGRectMake((has_mini_img?35:0)+1.0*cell.indentationWidth,
                                           22,
                                           14,14);
        bottomImageView.tag = BOTTOM_IMAGE_TAG;
        bottomImageView.opaque=TRUE;
        [cell.contentView addSubview:bottomImageView];
        
        coverImgView=[[UIImageView alloc] initWithImage:nil];
        coverImgView.frame= CGRectMake(0,1,34,34);
        coverImgView.contentMode=UIViewContentModeScaleAspectFit;
        coverImgView.tag = COVER_IMAGE_TAG;
        coverImgView.opaque=TRUE;
        [cell.contentView addSubview:coverImgView];
        
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
        bottomLabel = (CBAutoScrollLabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
        bottomImageView = (UIImageView *)[cell viewWithTag:BOTTOM_IMAGE_TAG];
        coverImgView = (UIImageView *)[cell viewWithTag:COVER_IMAGE_TAG];
        actionView = (UIButton *)[cell viewWithTag:ACT_IMAGE_TAG];
        secActionView = (UIButton *)[cell viewWithTag:SECACT_IMAGE_TAG];
    }
    actionView.hidden=TRUE;
    secActionView.hidden=TRUE;
    
    if (darkMode) {
        topLabel.textColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:1 green:1 blue:1 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.6 green:0.6 blue:0.6 alpha:1.0];
        //bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
    } else {
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        //bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0];
    }
    
    topLabel.frame= CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                               0,
                               tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-(has_mini_img?35:0)-PRI_SEC_ACTIONS_IMAGE_SIZE,
                               22);
    bottomLabel.frame = CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                                   22,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-(has_mini_img?35:0)-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                   18);
    bottomLabel.text=@""; //default value
    bottomImageView.image=nil;
    coverImgView.image=nil;
    
    cell.accessoryType = UITableViewCellAccessoryNone;
        
    cellValue=cur_db_entries[section][indexPath.row].label;
    int colFactor;
    //update downloaded if needed
    if(cur_db_entries[section][indexPath.row].downloaded==-1) {
        NSString *pathToCheck=nil;
        
        if (cur_db_entries[section][indexPath.row].fullpath)
            pathToCheck=[NSString stringWithFormat:@"%@/%@",NSHomeDirectory(),cur_db_entries[section][indexPath.row].fullpath];
        if (pathToCheck) {
            if ([mFileMngr fileExistsAtPath:pathToCheck]) cur_db_entries[section][indexPath.row].downloaded=1;
            else cur_db_entries[section][indexPath.row].downloaded=0;
        } else cur_db_entries[section][indexPath.row].downloaded=0;
    }
    
    if(cur_db_entries[section][indexPath.row].downloaded==1) {
        colFactor=1;
    } else colFactor=0;
    
    if (cur_db_entries[section][indexPath.row].isFile) { //FILE
        if (colFactor==0) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0];
        topLabel.frame= CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                                   0,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-(has_mini_img?35:0),
                                   22);
        if (cur_db_entries[section][indexPath.row].downloaded==1) {
            if (cur_db_entries[section][indexPath.row].rating==-1) {
                DBHelper::getFileStatsDBmod(
                                            cur_db_entries[section][indexPath.row].label,
                                            cur_db_entries[section][indexPath.row].fullpath,
                                            &cur_db_entries[section][indexPath.row].playcount,
                                            &cur_db_entries[section][indexPath.row].rating,
                                            &cur_db_entries[section][indexPath.row].song_length,
                                            &cur_db_entries[section][indexPath.row].channels_nb,
                                            &cur_db_entries[section][indexPath.row].songs);
            }
            if (cur_db_entries[section][indexPath.row].rating>0) bottomImageView.image=[UIImage imageNamed:ratingImg[RATING_IMG(cur_db_entries[section][indexPath.row].rating)]];
            
            NSString *bottomStr;
            if (cur_db_entries[section][indexPath.row].song_length>0)
                bottomStr=[NSString stringWithFormat:@"%02d:%02d",cur_db_entries[section][indexPath.row].song_length/1000/60,(cur_db_entries[section][indexPath.row].song_length/1000)%60];
            else bottomStr=@"--:--";
            if (cur_db_entries[section][indexPath.row].channels_nb)
                bottomStr=[NSString stringWithFormat:@"%@・%02dch",bottomStr,cur_db_entries[section][indexPath.row].channels_nb];
            else bottomStr=[NSString stringWithFormat:@"%@・--ch",bottomStr];
            /*if (cur_db_entries[section][indexPath.row].songs) {
                if (cur_db_entries[section][indexPath.row].songs==1) bottomStr=[NSString stringWithFormat:@"%@|1 song",bottomStr];
                else bottomStr=[NSString stringWithFormat:@"%@|%d songs",bottomStr,cur_db_entries[section][indexPath.row].songs];
            }
            else bottomStr=[NSString stringWithFormat:@"%@|- song",bottomStr];*/
            bottomStr=[NSString stringWithFormat:@"%@・Pl:%d",bottomStr,cur_db_entries[section][indexPath.row].playcount];
            
            bottomLabel.text=[NSString stringWithFormat:@"%@・%@",cur_db_entries[section][indexPath.row].info,bottomStr];
            
            bottomLabel.frame = CGRectMake((has_mini_img?35:0)+ 1.0 * cell.indentationWidth+20,
                                           22,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-20-(has_mini_img?35:0),
                                           18);
        } else {
            bottomLabel.text=cur_db_entries[section][indexPath.row].info;
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
        actionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                      0,
                                      PRI_SEC_ACTIONS_IMAGE_SIZE,
                                      PRI_SEC_ACTIONS_IMAGE_SIZE);
        actionView.enabled=YES;
        actionView.hidden=NO;
        
        if (cur_db_entries[section][indexPath.row].img_URL) {
            coverImgView.image = [imagesCache getImageWithURL:cur_db_entries[section][indexPath.row].img_URL
                                                           prefix:@"SMSP_mini"
                                                             size:CGSizeMake(34.0f, 34.0f)
                                                   forUIImageView:coverImgView];
            //coverImgView.contentMode=UIViewContentModeScaleAspectFit;
        }
    } else { // DIR
        bottomLabel.frame = CGRectMake((has_mini_img?35:0)+ 1.0 * cell.indentationWidth,
                                       22,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-(has_mini_img?35:0),
                                       18);
        if (cur_db_entries[section][indexPath.row].info) {
            bottomLabel.text=[NSString stringWithFormat:@"%@",cur_db_entries[section][indexPath.row].info];
        } else {
            bottomLabel.text=nil;
        }
        topLabel.frame= CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                                   0,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-(has_mini_img?35:0),
                                   22);
        if (darkMode) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:1.0f alpha:1.0f];
        else topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
        
        if (cur_db_entries[section][indexPath.row].img_URL) {
            coverImgView.image = [imagesCache getImageWithURL:cur_db_entries[section][indexPath.row].img_URL
                                                           prefix:@"SMSP_mini"
                                                             size:CGSizeMake(34.0f, 34.0f)
                                                   forUIImageView:coverImgView];
            coverImgView.contentMode=UIViewContentModeScaleAspectFit;
        }
        
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    topLabel.text = cellValue;
    
    return cell;
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
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    
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
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    
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
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
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
        childController = [[RootViewControllerSMSPWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
        //set new title
        childController.title = cur_db_entries[section][indexPath.row].fullpath;
        // Set new directory
        ((RootViewControllerSMSPWebParser*)childController)->browse_depth = browse_depth+1;
        ((RootViewControllerSMSPWebParser*)childController)->detailViewController=detailViewController;
        ((RootViewControllerSMSPWebParser*)childController)->downloadViewController=downloadViewController;
        ((RootViewControllerSMSPWebParser*)childController)->mWebBaseURL=cur_db_entries[section][indexPath.row].URL;
        
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
