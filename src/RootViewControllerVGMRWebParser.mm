//
//  RootViewControllerVGMRWebParser.mm
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
static volatile int mPopupAnimation=0;

#import "AppDelegate_Phone.h"
#import "RootViewControllerVGMRWebParser.h"
#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
#import "WebBrowser.h"
#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];

#import "TFHpple.h"

#import "QuartzCore/CAAnimation.h"
#import "TTFadeAnimator.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;


@implementation RootViewControllerVGMRWebParser

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


- (void)viewDidLoad {
    clock_t start_time,end_time;
    start_time=clock();
    childController=NULL;
    
    indexTitleMode=0;
    
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
    
    dbWEB_hasFiles=search_dbWEB_hasFiles=0;
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    if (mSearch) {
        search_dbWEB=1;
        
        if (search_dbWEB_nb_entries) {
            search_dbWEB_nb_entries=0;
            free(search_dbWEB_entries_data);
        }
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
        {@"Top Packs",@"https://vgmrips.net/packs/top"},
        {@"Latest Packs",@"https://vgmrips.net/packs/latest"},
        {@"Sound Chips",@"https://vgmrips.net/packs/chips"},
        {@"Composers",@"https://vgmrips.net/packs/composers"},
        {@"Companies",@"https://vgmrips.net/packs/companies"},
        {@"Systems",@"https://vgmrips.net/packs/systems"}
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
    
    char str[1024];
    index=-1;
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_categ_entry *wentry = (t_categ_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        sprintf(str,"%s",[wentry->category UTF8String]);
        
        previndex=index;
        index=0;
        if (indexTitleMode) {
            if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
            if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
        }
        //sections are determined 'on the fly' since result set is already sorted
        if (previndex!=index) {
            if (previndex>index) {
                //NSLog(@"********* %s",str);
            } else dbWEB_entries[index]=&(dbWEB_entries_data[dbWEB_entries_index]);
        }
        
        dbWEB_entries[index][dbWEB_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",str];
                
        dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%s",str];//@"VGMRips";
                
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
    
    dbWEB_hasFiles=search_dbWEB_hasFiles=0;
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    if (mSearch) {
        search_dbWEB=1;
        
        if (search_dbWEB_nb_entries) {
            search_dbWEB_nb_entries=0;
            free(search_dbWEB_entries_data);
        }
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
                
    if ( ([mWebBaseURL rangeOfString:@"https://vgmrips.net/packs/system/"].location!=NSNotFound) ||
         ([mWebBaseURL rangeOfString:@"https://vgmrips.net/packs/composer/"].location!=NSNotFound) ||
         ([mWebBaseURL rangeOfString:@"https://vgmrips.net/packs/chip/"].location!=NSNotFound) ||
         ([mWebBaseURL rangeOfString:@"/developed"].location!=NSNotFound)
         ){
        ///////////////////////////////////////////////////////////////////////:
        // Entries for a given system / chip / composer
        ///////////////////////////////////////////////////////////////////////:
                
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
                
        NSArray *arr_pageNb=[doc searchWithXPathQuery:@"((/html/body//select[@class='form-control'])[last()]/option)[last()]"];
        
        TFHppleElement *el=[arr_pageNb objectAtIndex:0];
        if (el) {
            int pageNb=[[el text] intValue];
            //NSLog(@"Page nb: %d",pageNb);
            
            we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*pageNb*20);
            int we_index=0;
            
            
            /*NSURLSessionConfiguration *configuration = [NSURLSessionConfiguration defaultSessionConfiguration];
            AFURLSessionManager *manager = [[AFURLSessionManager alloc] initWithSessionConfiguration:configuration];

            NSURL *URL = [NSURL URLWithString:@"http://example.com/download.zip"];
            NSURLRequest *request = [NSURLRequest requestWithURL:URL];

            NSURLSessionDownloadTask *downloadTask = [manager downloadTaskWithRequest:request progress:nil destination:^NSURL *(NSURL *targetPath, NSURLResponse *response) {
                NSURL *documentsDirectoryURL = [[NSFileManager defaultManager] URLForDirectory:NSDocumentDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil];
                return [documentsDirectoryURL URLByAppendingPathComponent:[response suggestedFilename]];
            } completionHandler:^(NSURLResponse *response, NSURL *filePath, NSError *error) {
                NSLog(@"File downloaded to: %@", filePath);
            }];
            [downloadTask resume];*/
            
            
            
            
            
            
            
            for (int i=0;i<pageNb;i++) {
                [self flushMainLoop];
                [self updateWaitingDetail:[NSString stringWithFormat:@"%d/%d",i+1,pageNb]];
                [self flushMainLoop];
                
                if (i>0) {
                    url = [NSURL URLWithString:[NSString stringWithFormat:@"%@?p=%d",mWebBaseURL,i+1]];
                    urlData = [NSData dataWithContentsOfURL:url];
                    doc       = [[TFHpple alloc] initWithHTMLData:urlData];
                }
                                                                            
                NSArray *arr_url=[doc searchWithXPathQuery:@"/html/body//div[@class='result row']//a[@class='download']/@href"];
                NSArray *arr_details=[doc searchWithXPathQuery:@"/html/body//div[@class='result row']//a[@class='download']/small/text()"];
                NSArray *arr_name=[doc searchWithXPathQuery:@"/html/body//div[@class='result row']//img/@alt"];
                NSArray *arr_img=[doc searchWithXPathQuery:@"/html/body//div[@class='result row']//img/@src"];
                NSArray *arr_companies=[doc searchWithXPathQuery:@"/html/body//div[@class='result row']//tr[@class='publishers']//a[last()]/text()"];
                
                if (arr_url&&[arr_url count]) {
                    
                    for (int j=0;j<[arr_url count];j++) {
                        TFHppleElement *el=[arr_url objectAtIndex:j];
                        we[we_index].file_URL=[NSString stringWithString:[el text]];
                        
                        el=[arr_img objectAtIndex:j];
                        we[we_index].file_img_URL=[NSString stringWithString:[el text]];
                        
                        el=[arr_name objectAtIndex:j];
                        we[we_index].file_name=[[[el text] stringByReplacingOccurrencesOfString:@"/" withString:@"-"]  stringByReplacingOccurrencesOfString:@"?" withString:@""];
                        
                        el=[arr_details objectAtIndex:j];
                        we[we_index].file_details=[NSString stringWithString:[el raw]];
                        
                        el=[arr_companies objectAtIndex:j];
                        we[we_index].file_company=[NSString stringWithString:[el raw]];
                        
                        we[we_index].file_type=1;
                        
                        [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                        
                        we_index++;
                    }
                }
            }
        } else we=NULL;
    } else if ([mWebBaseURL isEqualToString:@"https://vgmrips.net/packs/companies"]) {
        ///////////////////////////////////////////////////////////////////////:
        // Companies
        ///////////////////////////////////////////////////////////////////////:
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
        
        NSArray *arr_tr=[doc searchWithXPathQuery:@"/html/body//table//tr[position()>1]"];
        /*NSArray *arr_img=[doc searchWithXPathQuery:@"/html/body//div[@class='system']//div[@class='pic']//a[child::img|child::div[@class='icon']]"];
        NSArray *arr_details=[doc searchWithXPathQuery:@"/html/body//div[@class='system']//div[@class='pic']//a//div[@class='overlay']/span/text()"];
        */
        if (arr_tr&&[arr_tr count]) {
            we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[arr_tr count]);
            int we_index=0;
            for (int j=0;j<[arr_tr count];j++) {
                TFHppleElement *el=[arr_tr objectAtIndex:j];
                //NSLog(@"%@",[el raw]);
                if ([el hasChildren]) {
                    NSArray *el_childs=[el childrenWithTagName:@"td"];
                    for (int i=0;i<[el_childs count];i++) {
                        TFHppleElement *elc=[el_childs objectAtIndex:i];
                        
                        if ([[elc objectForKey:@"class"] isEqualToString:@"link"]) {
                            TFHppleElement *elc_a=[elc firstChildWithTagName:@"a"];
                            //NSLog(@"found title: %@",[elc_a objectForKey:@"title"]);
                            we[we_index].file_name=[NSString stringWithString:[elc_a objectForKey:@"title"]];
                            elc_a=[elc firstChildWithTagName:@"div"];
                            if (elc_a) {
                                elc_a=[elc_a firstChildWithTagName:@"a"];
                                if (elc_a) {
                                    elc_a=[elc_a firstChildWithTagName:@"img"];
                                    if (elc_a) {
                                        //NSLog(@"found image: %@",[elc_a objectForKey:@"src"]);
                                        we[we_index].file_img_URL=[NSString stringWithString:[elc_a objectForKey:@"src"]];
                                    }
                                }
                            }
                            //check in the 1st a tag for title & div then img for an image (optional)
                        } else {
                        TFHppleElement *elc_a=[elc firstChildWithTagName:@"span"];
                            if (elc_a) {
                                elc_a=[elc_a firstChildWithTagName:@"a"];
                                if (elc_a && [elc_a objectForKey:@"href"] && ([[elc_a objectForKey:@"href"] rangeOfString:@"/developed"].location!=NSNotFound)) {
                                    //NSLog(@"found link: %@",[elc_a objectForKey:@"href"]);
                                    we[we_index].file_URL=[elc_a objectForKey:@"href"];
                                    //NSLog(@"count: %@",[elc_a text]);
                                    
                                    if ([[elc_a text] isEqualToString:@"1"]) we[we_index].file_details=[NSString stringWithFormat:@"1 pack"];
                                    else we[we_index].file_details=[NSString stringWithFormat:@"%@ packs",[elc_a text]];
                                                                        
                                    we[we_index].file_type=0;
                                    [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                                    
                                    we_index++;
                                }
                            }
                        }
                    }
                }
            }
        } else we=NULL;
        
    } else if ([mWebBaseURL isEqualToString:@"https://vgmrips.net/packs/systems"]) {
        ///////////////////////////////////////////////////////////////////////:
        // Systems
        ///////////////////////////////////////////////////////////////////////:
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
        
        NSArray *arr_url=[doc searchWithXPathQuery:@"/html/body//div[@class='system']//div[@class='name']//a"];
        NSArray *arr_img=[doc searchWithXPathQuery:@"/html/body//div[@class='system']//div[@class='pic']//a[child::img|child::div[@class='icon']]"];
        NSArray *arr_details=[doc searchWithXPathQuery:@"/html/body//div[@class='system']//div[@class='pic']//a//div[@class='overlay']/span/text()"];
        
        if (arr_url&&[arr_url count]) {
            we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[arr_url count]);
            for (int j=0;j<[arr_url count];j++) {
                TFHppleElement *el=[arr_url objectAtIndex:j];
                we[j].file_URL=[NSString stringWithString:[el objectForKey:@"href"]];
                
                we[j].file_name=[[[el text] stringByReplacingOccurrencesOfString:@"/" withString:@"-"]  stringByReplacingOccurrencesOfString:@"?" withString:@""];
                
                el=[arr_details objectAtIndex:j];
                we[j].file_details=[NSString stringWithString:[el raw]];
                
                el=[[arr_img objectAtIndex:j] firstChildWithTagName:@"img"];
                if (el) we[j].file_img_URL=[NSString stringWithString:[el objectForKey:@"src"]];
                else we[j].file_img_URL=nil;
                
                we[j].file_company=nil;
                
                we[j].file_type=0;
                
                /*el=[arr_name objectAtIndex:j];
                ;
                el=[arr_companies objectAtIndex:j];
                we[j].file_company=[NSString stringWithString:[el raw]];*/
                
                [tmpArray addObject:[NSValue valueWithPointer:&(we[j])]];
            }
        } else we=NULL;
        
    } else if ([mWebBaseURL isEqualToString:@"https://vgmrips.net/packs/chips"]) {
        ///////////////////////////////////////////////////////////////////////:
        // sound chips
        ///////////////////////////////////////////////////////////////////////:
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
        
        NSArray *arr_url=[doc searchWithXPathQuery:@"/html/body//div[@class='chip']//a[last()]"];
        NSArray *arr_img=[doc searchWithXPathQuery:@"/html/body//div[@class='chip']//img"];
        NSArray *arr_details=[doc searchWithXPathQuery:@"/html/body//div[@class='chip']//span/text()"];
        
        if (arr_url&&[arr_url count]) {
            we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[arr_url count]);
            for (int j=0;j<[arr_url count];j++) {
                TFHppleElement *el=[arr_url objectAtIndex:j];
                we[j].file_URL=[NSString stringWithString:[el objectForKey:@"href"]];
                
                we[j].file_name=[[[el text] stringByReplacingOccurrencesOfString:@"/" withString:@"-"]  stringByReplacingOccurrencesOfString:@"?" withString:@""];
                
                el=[arr_details objectAtIndex:j];
                if ([[el raw] isEqualToString:@"1"]) we[j].file_details=[NSString stringWithFormat:@"1 pack"];
                else we[j].file_details=[NSString stringWithFormat:@"%@ packs",[el raw]];
                
                el=[arr_img objectAtIndex:j];
                if (el) we[j].file_img_URL=[NSString stringWithString:[el objectForKey:@"src"]];
                else we[j].file_img_URL=nil;
                
                we[j].file_company=nil;
                
                we[j].file_type=0;
                
                /*el=[arr_name objectAtIndex:j];
                ;
                el=[arr_companies objectAtIndex:j];
                we[j].file_company=[NSString stringWithString:[el raw]];*/
                
                [tmpArray addObject:[NSValue valueWithPointer:&(we[j])]];
            }
        } else we=NULL;
        
    } else if ([mWebBaseURL isEqualToString:@"https://vgmrips.net/packs/composers"]) {
        ///////////////////////////////////////////////////////////////////////:
        // composers
        ///////////////////////////////////////////////////////////////////////:
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
        
        NSArray *arr_url=[doc searchWithXPathQuery:@"/html/body//li[@class='composer']/a"];
        NSArray *arr_name=[doc searchWithXPathQuery:@"/html/body//li[@class='composer']/a/text()[3]"];
        NSArray *arr_details=[doc searchWithXPathQuery:@"/html/body//li[@class='composer']/a//span[@class='badge']/text()"];
        NSArray *arr_ratings=[doc searchWithXPathQuery:@"/html/body//li[@class='composer']/a//div/@class"];
        
        if (arr_url&&[arr_url count]) {
            we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[arr_url count]);
            for (int j=0;j<[arr_url count];j++) {
                TFHppleElement *el=[arr_url objectAtIndex:j];
                we[j].file_URL=[NSString stringWithString:[el objectForKey:@"href"]];
                
                el=[arr_name objectAtIndex:j];
                we[j].file_name=[[[[[[el raw] stringByReplacingOccurrencesOfString:@"/" withString:@"-"]  stringByReplacingOccurrencesOfString:@"?" withString:@""] stringByReplacingOccurrencesOfString:@"\n" withString:@""] stringByReplacingOccurrencesOfString:@"\t" withString:@""] stringByReplacingOccurrencesOfString:@"&#13;" withString:@""];
                
                el=[arr_details objectAtIndex:j];
                if ([[el raw] isEqualToString:@"1"]) we[j].file_details=[NSString stringWithFormat:@"1 pack"];
                else we[j].file_details=[NSString stringWithFormat:@"%@ packs",[el raw]];
                
                el=[arr_ratings objectAtIndex:j];
                
                we[j].file_company=nil;
                
                we[j].file_type=0;
                                                
                [tmpArray addObject:[NSValue valueWithPointer:&(we[j])]];
            }
        } else we=NULL;
        
    } else if (([mWebBaseURL isEqualToString:@"https://vgmrips.net/packs/top"])||
        ([mWebBaseURL isEqualToString:@"https://vgmrips.net/packs/latest"])) {
        ///////////////////////////////////////////////////////////////////////:
        // Latest & Top entries
        ///////////////////////////////////////////////////////////////////////:
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
        
        int pageNb=1; //top 20 to start
        //NSLog(@"Page nb: %d",pageNb);
        
        we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*pageNb*20);
        int we_index=0;
        for (int i=0;i<pageNb;i++) {
            [self flushMainLoop];
            [self updateWaitingDetail:[NSString stringWithFormat:@"%d/%d",i+1,pageNb]];
            [self flushMainLoop];
            
            if (i>0) {
                url = [NSURL URLWithString:[NSString stringWithFormat:@"%@?p=%d",mWebBaseURL,i]];
                urlData = [NSData dataWithContentsOfURL:url];
                doc       = [[TFHpple alloc] initWithHTMLData:urlData];
            }
            NSArray *arr_url=[doc searchWithXPathQuery:@"/html/body//div[@class='result row']//a[@class='download']/@href"];
            NSArray *arr_details=[doc searchWithXPathQuery:@"/html/body//div[@class='result row']//a[@class='download']/small/text()"];
            NSArray *arr_name=[doc searchWithXPathQuery:@"/html/body//div[@class='result row']//img/@alt"];
            NSArray *arr_img=[doc searchWithXPathQuery:@"/html/body//div[@class='result row']//img/@src"];
            NSArray *arr_companies=[doc searchWithXPathQuery:@"/html/body//div[@class='result row']//tr[@class='publishers']//a[last()]/text()"];
            
            if (arr_url&&[arr_url count]) {
                
                for (int j=0;j<[arr_url count];j++) {
                    TFHppleElement *el=[arr_url objectAtIndex:j];
                    we[we_index].file_URL=[NSString stringWithString:[el text]];
                    
                    el=[arr_img objectAtIndex:j];
                    we[we_index].file_img_URL=[NSString stringWithString:[el text]];
                    
                    el=[arr_name objectAtIndex:j];
                    we[we_index].file_name=[[[el text] stringByReplacingOccurrencesOfString:@"/" withString:@"-"]  stringByReplacingOccurrencesOfString:@"?" withString:@""];
                    
                    el=[arr_details objectAtIndex:j];
                    we[we_index].file_details=[NSString stringWithString:[el raw]];
                    
                    el=[arr_companies objectAtIndex:j];
                    we[we_index].file_company=[NSString stringWithString:[el raw]];
                    
                    we[we_index].file_type=1;
                    
                    [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                    
                    we_index++;
                }
            }
        }
    }
        
    if (indexTitleMode) {
        sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
            NSString *str1=[((t_web_file_entry*)[obj1 pointerValue])->file_URL lastPathComponent];
            NSString *str2=[((t_web_file_entry*)[obj2 pointerValue])->file_URL lastPathComponent];
            return [str1 caseInsensitiveCompare:str2];
        }];
    } else {
        sortedArray=tmpArray;
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

    char str[1024];
    index=-1;
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_web_file_entry *wef = (t_web_file_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        //NSLog(@"%@",wef->file_name);
        sprintf(str,"%s",[[wef->file_name stringByRemovingPercentEncoding] UTF8String]);
        
        //NSLog(@"%@",wef->file_size);
        previndex=index;
        index=0;
        if (indexTitleMode) {
            if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
            if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
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
                
        if (wef->file_type==1) dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[NSString stringWithFormat:@"Documents/VGMRips/%@/%@.zip",wef->file_company,wef->file_name];
        else dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@",wef->file_name];
        
        if (wef->file_URL) dbWEB_entries[index][dbWEB_entries_count[index]].URL=[NSString stringWithString:wef->file_URL];
        
        if (wef->file_img_URL) dbWEB_entries[index][dbWEB_entries_count[index]].img_URL=[NSString stringWithString:wef->file_img_URL];
                
        dbWEB_entries[index][dbWEB_entries_count[index]].isFile=wef->file_type;
        dbWEB_entries[index][dbWEB_entries_count[index]].downloaded=-1;
        if (wef->file_type) dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"%@・%@",wef->file_company, [wef->file_details stringByReplacingOccurrencesOfString:@"&#13;\n" withString:@""]];
        else  dbWEB_entries[index][dbWEB_entries_count[index]].info=[wef->file_details stringByReplacingOccurrencesOfString:@"&#13;\n" withString:@""];
        
        dbWEB_entries[index][dbWEB_entries_count[index]].rating=-1;
        dbWEB_entries[index][dbWEB_entries_count[index]].playcount=-1;
        dbWEB_entries_count[index]++;
        dbWEB_entries_index++;
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
    if (childController) [(RootViewControllerVGMRWebParser*)childController refreshViewAfterDownload];
    else {
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
    UILabel *bottomLabel;
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
        bottomImageView.frame = CGRectMake((has_mini_img?35:0)+1.0*cell.indentationWidth,
                                           22,
                                           14,14);
        bottomImageView.tag = BOTTOM_IMAGE_TAG;
        bottomImageView.opaque=TRUE;
        [cell.contentView addSubview:bottomImageView];
        
        coverImgView=[[UIImageView alloc] initWithImage:nil];
        coverImgView.frame= CGRectMake(0,1,34,34);
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
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
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
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
    } else {
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0];
    }
    
    topLabel.frame= CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                               0,
                               tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-(has_mini_img?35:0),
                               22);
    bottomLabel.frame = CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                                   22,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-(has_mini_img?35:0),
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
                                                           prefix:@"vgmrips_mini"
                                                             size:CGSizeMake(34.0f, 34.0f)
                                                   forUIImageView:coverImgView];
        }
    } else { // DIR
        bottomLabel.frame = CGRectMake((has_mini_img?35:0)+ 1.0 * cell.indentationWidth,
                                       22,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-(has_mini_img?35:0),
                                       18);
        if (cur_db_entries[section][indexPath.row].info) {
            bottomLabel.text=[NSString stringWithFormat:@"%@・%@",cur_db_entries[section][indexPath.row].info,(cur_db_entries[section][indexPath.row].URL?cur_db_entries[section][indexPath.row].URL:@"")];
        } else {
            bottomLabel.text=[NSString stringWithString:(cur_db_entries[section][indexPath.row].URL?cur_db_entries[section][indexPath.row].URL:@"")];
        }
        topLabel.frame= CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                                   0,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-(has_mini_img?35:0),
                                   22);
        if (darkMode) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:1.0f alpha:1.0f];
        else topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
        
        if (cur_db_entries[section][indexPath.row].img_URL) {
              coverImgView.image = [imagesCache getImageWithURL:cur_db_entries[section][indexPath.row].img_URL
                                                           prefix:@"vgmrips_mini"
                                                             size:CGSizeMake(34.0f, 34.0f)
                                                   forUIImageView:coverImgView];
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
        childController = [[RootViewControllerVGMRWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
        //set new title
        childController.title = cur_db_entries[section][indexPath.row].fullpath;
        // Set new directory
        ((RootViewControllerVGMRWebParser*)childController)->browse_depth = browse_depth+1;
        ((RootViewControllerVGMRWebParser*)childController)->detailViewController=detailViewController;
        ((RootViewControllerVGMRWebParser*)childController)->downloadViewController=downloadViewController;
        ((RootViewControllerVGMRWebParser*)childController)->mWebBaseURL=cur_db_entries[section][indexPath.row].URL;
        
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
