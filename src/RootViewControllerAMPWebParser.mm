//
//  RootViewControllerAMPWebParser.mm
//  modizer
//
//  Created by Yohann Magnien on 07/05/21.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#import "RootViewControllerAMPWebParser.h"
#import "ModizFileHelper.h"

enum {
    BROWSE_DEFAULT=0,
    AMP_LINK_NONE,
    AMP_LINK_COMPOSERS,
    AMP_LINK_GROUPS,
    AMP_LINK_MODULES,
    AMP_LINK_SEARCH,
    AMP_LINK_BROWSE,
    AMP_LINK_LUCKY,
    AMP_LINK_BROWSE_COMPOSERS,
    AMP_LINK_BROWSE_GROUPS,
    AMP_LINK_BROWSE_MODULES,
    AMP_LINK_COMPOSERS_LIST,
    AMP_LINK_SEARCH_COMPOSERS_LIST,
    AMP_LINK_COMPOSER_DETAILS,
    AMP_LINK_SEARCH_MODULES_LIST,
    AMP_LINK_MODULES_LIST,
    AMP_LINK_SEARCH_GROUPS_LIST,
    AMP_LINK_GROUPS_LIST,
    AMP_LINK_INTERVIEW,
    AMP_LINK_MODULE_FILE,
    AMP_LINK_GROUP_DETAILS,
    AMP_LINK_COUNTRY_DETAILS,
};


@implementation RootViewControllerAMPWebParser

@synthesize browse_subMode;

typedef struct {
    NSString *file_URL;
    NSString *file_name;
    NSMutableAttributedString *file_nameAttr;
    NSString *composer;
    float file_rating;
    int entries_nb;
    NSString *file_details;
    NSString *file_img_URL;
    char url_type;
} t_web_file_entry;


int qsortAMP_entries_alpha(const void *entryA, const void *entryB) {
    NSString *strA,*strB;
    NSComparisonResult res;
    strA=((t_WEB_browse_entry*)entryA)->label;
    strB=((t_WEB_browse_entry*)entryB)->label;
    res=[strA localizedCaseInsensitiveCompare:strB];
    if (res==NSOrderedAscending) return -1;
    if (res==NSOrderedSame) return 0;
    return 1; //NSOrderedDescending
}

int qsortAMP_entries_rating_or_entries(const void *entryA, const void *entryB) {
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
    
//    [tableView reloadData];
}

- (void)onBackTapped {

    if ((browse_depth<=0)||(browse_subMode==AMP_LINK_INTERVIEW)) {
        [self.navigationController popViewControllerAnimated:YES];
    } else {
        browse_depth--;
        
        self.title=[arr_VC_title lastObject];
        [arr_VC_title removeLastObject];
        mWebBaseURL=[arr_VC_URL lastObject];
        [arr_VC_URL removeLastObject];
        browse_subMode=[(NSNumber*)[arr_VC_Mode lastObject] intValue];
        [arr_VC_Mode removeLastObject];
        
        mSearchText=[arr_VC_search lastObject];
        [arr_VC_search removeLastObject];
        if ([mSearchText length]==0) {
            mSearchText=nil;
            mSearch=0;
            search_dbWEB=0;  //reset to ensure search_dbWEB is not used by default
        } else {
            mSearch=1;
        }
        sBar.text=mSearchText;
        
        sort_mode=0;
        entries_noMoreToLoad=false;
        shouldReload=false;
        
        shouldFillKeys=1;
        
        
        dbWEB_entries=NULL;
        search_dbWEB_entries=NULL;
        
        dbWEB_nb_entries=0;
        search_dbWEB_nb_entries=0;
        search_dbWEB_entries_count=0;
        
        search_dbWEB_hasFiles=0;
        dbWEB_hasFiles=0;
        
        
        mClickedPrimAction=0;
        

        arr_url_handleList=[NSMutableArray array];
        arr_url_realnameList=[NSMutableArray array];
        arr_url_countryList=[NSMutableArray array];
        arr_url_groupsList=[NSMutableArray array];
        arr_url_groupsLogoList=[NSMutableArray array];
        
        arr_url_fileList=[NSMutableArray array];
        arr_url_composerList=[NSMutableArray array];
        arr_url_formatList=[NSMutableArray array];
        arr_url_sizeList=[NSMutableArray array];
        
        arr_current_fetch_position=0;
        
        htmlData=nil;
        
        [self updateWaitingDetail:@""];
        [self showWaiting];
        [self flushMainLoop];
        
        [self fillMoreKeys];
    }
}

- (void)viewDidLoad {
    START_PROFILE
    [super viewDidLoad];
    
    UIImage *image = [UIImage systemImageNamed:@"chevron.backward"];

    UIBarButtonItem *backButton =
        [[UIBarButtonItem alloc] initWithImage:image
                                         style:UIBarButtonItemStylePlain
                                        target:self
                                        action:@selector(onBackTapped)];

    self.navigationItem.leftBarButtonItem = backButton;
    
    arr_VC_title=[NSMutableArray array];
    arr_VC_URL=[NSMutableArray array];
    arr_VC_Mode=[NSMutableArray array];
    arr_VC_search=[NSMutableArray array];
    
    sort_mode=0;
    entries_noMoreToLoad=false;
    shouldReload=false;
    
    if (mWebBaseURL==nil) mWebBaseURL=@"";

    arr_url_handleList=[NSMutableArray array];
    arr_url_realnameList=[NSMutableArray array];
    arr_url_countryList=[NSMutableArray array];
    arr_url_groupsList=[NSMutableArray array];
    arr_url_groupsLogoList=[NSMutableArray array];
    
    arr_url_fileList=[NSMutableArray array];
    arr_url_composerList=[NSMutableArray array];
    arr_url_formatList=[NSMutableArray array];
    arr_url_sizeList=[NSMutableArray array];
    
    arr_current_fetch_position=0;

    // Configure scroll indicators
    self.tableView.showsVerticalScrollIndicator=YES;
    self.tableView.showsHorizontalScrollIndicator=NO;
    self.tableView.scrollEnabled=YES;
    self.tableView.alwaysBounceVertical=YES;
    self.tableView.indicatorStyle=darkMode?UIScrollViewIndicatorStyleWhite:UIScrollViewIndicatorStyleDefault;

    // Ensure scroll indicators have space to display
    self.tableView.contentInsetAdjustmentBehavior=UIScrollViewContentInsetAdjustmentAutomatic;

//    if (browse_depth>=2) {
//        self.navigationItem.titleView=navbarTitle;
//
//        UITapGestureRecognizer *tapGesture =
//        [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(titleTap)];
//        [navbarTitle addGestureRecognizer:tapGesture];
//    }
    END_PROFILE
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];

    // Ensure scroll indicators remain visible
    // NOTE: Do NOT reset scrollIndicatorInsets here - they are managed by MiniPlayer
    self.tableView.showsVerticalScrollIndicator=YES;
    self.tableView.scrollEnabled=YES;
    self.tableView.indicatorStyle=darkMode?UIScrollViewIndicatorStyleWhite:UIScrollViewIndicatorStyleDefault;

}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];

    // FORCE scroll indicators to be visible - this is the final word!
    self.tableView.showsVerticalScrollIndicator=YES;
    self.tableView.showsHorizontalScrollIndicator=NO;
    self.tableView.scrollEnabled=YES;

    // Force scroll indicators to appear after view is fully laid out
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.tableView flashScrollIndicators];
    });
}

-(void) fillKeysCompleted {
    [super fillKeysCompleted];
    fillKeysInProgress=0;
    if (mSearch && (search_dbWEB_entries_count==0)&&(entries_noMoreToLoad==false)) {
        dispatch_async(dispatch_get_main_queue(), ^(void){
//            [self hideWaiting];
            [self fillMoreKeys];
            [tableView reloadData];
            [tableView layoutIfNeeded];
        });
    } else {
        dispatch_async(dispatch_get_main_queue(), ^(void){
//            [self hideWaiting];
            [tableView reloadData];
            [tableView layoutIfNeeded];
        });
    }
}

-(void) populateKeys {
//    MDZILog("populate with submode: %d",self.browse_subMode);
    
    if (browse_depth==0) {
        [self fillKeysWithRepoCateg];
        dispatch_async(dispatch_get_main_queue(), ^(void){
            [self fillKeysCompleted];
        });
    }
    else {
        switch (self.browse_subMode) {
            case AMP_LINK_COMPOSERS:{
                [self fillKeysWithModeCateg];
                dispatch_async(dispatch_get_main_queue(), ^(void){
                    [self fillKeysCompleted];
                });
            }
                break;
            case AMP_LINK_BROWSE_COMPOSERS:
            case AMP_LINK_BROWSE_GROUPS:
            case AMP_LINK_BROWSE_MODULES:{
                [self fillKeysWithBrowserIndex];
                dispatch_async(dispatch_get_main_queue(), ^(void){
                    [self fillKeysCompleted];
                });
            }
                break;
            default:
                [self fillKeysWithWEBSource];
                break;
        }
    }
}

-(void) fillMoreKeys {
    shouldFillKeys=1;
    
    // Cancel previous search timer to debounce
    [self.searchDebounceTimer invalidate];
    self.searchDebounceTimer = nil;

    // Schedule new search after delay
    self.searchDebounceTimer = [NSTimer scheduledTimerWithTimeInterval:0.5
                                                                 target:self
                                                               selector:@selector(fillKeys)
                                                               userInfo:nil
                                                                repeats:NO];
//    dispatch_async(dispatch_get_main_queue(), ^(void){
//        [self fillKeys];
//    });
}

-(void) fillKeys {
    if (fillKeysInProgress) return;
    fillKeysInProgress=1;
    if (shouldFillKeys) {
        shouldFillKeys=0;
        [self populateKeys];
    } else { //reset downloaded, rating & playcount flags
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].downloaded=-1;
            dbWEB_entries_data[i].rating=-1;
            dbWEB_entries_data[i].playcount=-1;
        }
        if (mSearch) {
            [self populateKeys];
        } else {
            dispatch_async(dispatch_get_main_queue(), ^(void){
                [self fillKeysCompleted];
            });
        }
    }
}

-(void) fillKeysWithRepoCateg {
    int dbWEB_entries_index;
    
    entries_noMoreToLoad=true;
    
    if (search_dbWEB_nb_entries) {
            for (int j=0;j<search_dbWEB_entries_count;j++) {
                search_dbWEB_entries[j].label=nil;
                search_dbWEB_entries[j].fullpath=nil;
                search_dbWEB_entries[j].URL=nil;
                search_dbWEB_entries[j].url_type=0;
                search_dbWEB_entries[j].info=nil;
                search_dbWEB_entries[j].img_URL=nil;
                search_dbWEB_entries[j].labelAttr=nil;
                search_dbWEB_entries[j].infoAttr=nil;
            }
            search_dbWEB_entries=NULL;
        search_dbWEB_nb_entries=0;
        free(search_dbWEB_entries_data);
    }
    dbWEB_hasFiles=search_dbWEB_hasFiles=0;
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    if (mSearch) {
        search_dbWEB=1;
        
        search_dbWEB_entries_data=(t_WEB_browse_entry*)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
        
        search_dbWEB_entries_count=0;
        if (dbWEB_entries_count) search_dbWEB_entries=search_dbWEB_entries_data;
            for (int j=0;j<dbWEB_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:dbWEB_entries[j].label]) {
                    search_dbWEB_entries[search_dbWEB_entries_count].label=dbWEB_entries[j].label;
                    search_dbWEB_entries[search_dbWEB_entries_count].downloaded=dbWEB_entries[j].downloaded;
                    search_dbWEB_entries[search_dbWEB_entries_count].rating=dbWEB_entries[j].rating;
                    search_dbWEB_entries[search_dbWEB_entries_count].playcount=dbWEB_entries[j].playcount;
                    search_dbWEB_entries[search_dbWEB_entries_count].fullpath=dbWEB_entries[j].fullpath;
                    search_dbWEB_entries[search_dbWEB_entries_count].URL=dbWEB_entries[j].URL;
                    search_dbWEB_entries[search_dbWEB_entries_count].img_URL=dbWEB_entries[j].img_URL;
                    search_dbWEB_entries[search_dbWEB_entries_count].isFile=dbWEB_entries[j].isFile;
                    search_dbWEB_entries[search_dbWEB_entries_count].url_type=dbWEB_entries[j].url_type;
                    search_dbWEB_entries[search_dbWEB_entries_count].info=dbWEB_entries[j].info;
                    search_dbWEB_entries[search_dbWEB_entries_count].labelAttr=dbWEB_entries[j].labelAttr;
                    search_dbWEB_entries[search_dbWEB_entries_count].infoAttr=dbWEB_entries[j].infoAttr;
                    search_dbWEB_entries_count++;
                    search_dbWEB_nb_entries++;
                }
            }
        return;
    }
    if (dbWEB_nb_entries) {
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].label=nil;
            dbWEB_entries_data[i].fullpath=nil;
            dbWEB_entries_data[i].URL=nil;
            dbWEB_entries_data[i].url_type=0;
            dbWEB_entries_data[i].info=nil;
            dbWEB_entries_data[i].img_URL=nil;
            dbWEB_entries_data[i].labelAttr=nil;
            dbWEB_entries_data[i].infoAttr=nil;
        }
        free(dbWEB_entries_data);dbWEB_entries_data=NULL;
        dbWEB_nb_entries=0;
    }
    
    typedef struct {
        NSString *category;
        NSString *url;
        char url_type;
    } t_categ_entry;
    NSArray *sortedArray;
    NSMutableArray *tmpArray=[[NSMutableArray alloc] init];
    t_categ_entry webs_entry[]= {
        {@"Composers",@"https://amp.dascene.net/newresult.php",AMP_LINK_COMPOSERS},
        {@"Groups",@"https://amp.dascene.net/newresult.php?request=groups&search=",AMP_LINK_SEARCH_GROUPS_LIST},
        {@"Modules",@"https://amp.dascene.net/newresult.php?request=module&search=",AMP_LINK_SEARCH_MODULES_LIST}
    };
    
    for (int i=0;i<sizeof(webs_entry)/sizeof(t_categ_entry);i++) [tmpArray addObject:[NSValue valueWithPointer:&webs_entry[i]]];
    
        sortedArray=tmpArray;
    dbWEB_nb_entries=(int)[sortedArray count];
    
    //2nd initialize array to receive entries
    dbWEB_entries_data=(t_WEB_browse_entry *)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    memset(dbWEB_entries_data,0,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    dbWEB_entries_index=0;
        dbWEB_entries_count=0;
        dbWEB_entries=dbWEB_entries_data;
    
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_categ_entry *wentry = (t_categ_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        
        dbWEB_entries[dbWEB_entries_count].label=[[NSString alloc] initWithFormat:@"%@",wentry->category];
        
        dbWEB_entries[dbWEB_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@",wentry->category];
        
        dbWEB_entries[dbWEB_entries_count].url_type=wentry->url_type;
        
        dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithString:wentry->url];
        
        dbWEB_entries[dbWEB_entries_count].isFile=0;
        
        dbWEB_entries_count++;
        dbWEB_entries_index++;
    }
    //populate entries
}

-(void) fillSearchWithEntries {
    if (search_dbWEB_nb_entries) {
            for (int j=0;j<search_dbWEB_entries_count;j++) {
                search_dbWEB_entries[j].label=nil;
                search_dbWEB_entries[j].fullpath=nil;
                search_dbWEB_entries[j].URL=nil;
                search_dbWEB_entries[j].info=nil;
                search_dbWEB_entries[j].img_URL=nil;
                search_dbWEB_entries[j].labelAttr=nil;
                search_dbWEB_entries[j].infoAttr=nil;
            }
            search_dbWEB_entries=NULL;
        search_dbWEB_nb_entries=0;
        free(search_dbWEB_entries_data);
    }
    search_dbWEB_hasFiles=0;
    
    search_dbWEB=1;
    
    search_dbWEB_entries_data=(t_WEB_browse_entry*)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    
        search_dbWEB_entries_count=0;
    if (dbWEB_entries_count) search_dbWEB_entries=search_dbWEB_entries_data;
    for (int j=0;j<dbWEB_entries_count;j++)  {
        if ([self searchStringRegExp:mSearchText sourceString:dbWEB_entries[j].label]) {
            search_dbWEB_entries[search_dbWEB_entries_count].label=dbWEB_entries[j].label;
            search_dbWEB_entries[search_dbWEB_entries_count].downloaded=dbWEB_entries[j].downloaded;
            search_dbWEB_entries[search_dbWEB_entries_count].rating=dbWEB_entries[j].rating;
            search_dbWEB_entries[search_dbWEB_entries_count].playcount=dbWEB_entries[j].playcount;
            search_dbWEB_entries[search_dbWEB_entries_count].fullpath=dbWEB_entries[j].fullpath;
            search_dbWEB_entries[search_dbWEB_entries_count].URL=dbWEB_entries[j].URL;
            search_dbWEB_entries[search_dbWEB_entries_count].isFile=dbWEB_entries[j].isFile;
            search_dbWEB_entries[search_dbWEB_entries_count].url_type=dbWEB_entries[j].url_type;
            search_dbWEB_entries[search_dbWEB_entries_count].info=dbWEB_entries[j].info;
            search_dbWEB_entries[search_dbWEB_entries_count].img_URL=dbWEB_entries[j].img_URL;
            search_dbWEB_entries[search_dbWEB_entries_count].labelAttr=dbWEB_entries[j].labelAttr;
            search_dbWEB_entries[search_dbWEB_entries_count].infoAttr=dbWEB_entries[j].infoAttr;
            
            
            search_dbWEB_entries_count++;
            search_dbWEB_nb_entries++;
        }
    }
}

-(void) fillKeysWithModeCateg {
    int dbWEB_entries_index;
    
    entries_noMoreToLoad=true;
    dbWEB_hasFiles=0;
    
    if (entries_noMoreToLoad && mSearch) {
        // in case of search, do not ask DB again => duplicate already found entries & filter them
        [self fillSearchWithEntries];
        return;
    }
    
    if (dbWEB_nb_entries) {
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].label=nil;
            dbWEB_entries_data[i].fullpath=nil;
            dbWEB_entries_data[i].URL=nil;
            dbWEB_entries_data[i].url_type=0;
            dbWEB_entries_data[i].info=nil;
            dbWEB_entries_data[i].img_URL=nil;
            dbWEB_entries_data[i].labelAttr=nil;
            dbWEB_entries_data[i].infoAttr=nil;
        }
        free(dbWEB_entries_data);dbWEB_entries_data=NULL;
        dbWEB_nb_entries=0;
    }
    
    typedef struct {
        NSString *category;
        NSString *url;
        char url_type;
    } t_categ_entry;
    NSArray *sortedArray;
    NSMutableArray *tmpArray=[[NSMutableArray alloc] init];
    t_categ_entry webs_entry[]= {
        {@"Search",@"https://amp.dascene.net/newresult.php",AMP_LINK_SEARCH},
        {@"Browse",@"https://amp.dascene.net/newresult.php",AMP_LINK_BROWSE},
        {@"Feeling Lucky",@"https://amp.dascene.net/newresult.php?request=list",AMP_LINK_LUCKY}
    };
    
    for (int i=0;i<sizeof(webs_entry)/sizeof(t_categ_entry);i++) [tmpArray addObject:[NSValue valueWithPointer:&webs_entry[i]]];
    
        sortedArray=tmpArray;
    dbWEB_nb_entries=[sortedArray count];
    
    //Feeling lucky is only for composers
    if (browse_subMode!=AMP_LINK_COMPOSERS) dbWEB_nb_entries--;
    
    //2nd initialize array to receive entries
    dbWEB_entries_data=(t_WEB_browse_entry *)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    memset(dbWEB_entries_data,0,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    dbWEB_entries_index=0;
        dbWEB_entries_count=0;
        dbWEB_entries=dbWEB_entries_data;
    
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_categ_entry *wentry = (t_categ_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        
        dbWEB_entries[dbWEB_entries_count].label=[[NSString alloc] initWithFormat:@"%@",wentry->category];
        
        dbWEB_entries[dbWEB_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@", wentry->category];
        
        switch (wentry->url_type) {
            case AMP_LINK_SEARCH:
                if (browse_subMode==AMP_LINK_COMPOSERS) {
                    dbWEB_entries[dbWEB_entries_count].url_type=AMP_LINK_SEARCH_COMPOSERS_LIST;
                    dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithFormat:@"%@?request=handle&search=",wentry->url];
                } else if (browse_subMode==AMP_LINK_GROUPS) {
                    dbWEB_entries[dbWEB_entries_count].url_type=AMP_LINK_SEARCH_GROUPS_LIST;
                    dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithFormat:@"%@?request=groups&search=",wentry->url];
                } else if (browse_subMode==AMP_LINK_MODULES) {
                    dbWEB_entries[dbWEB_entries_count].url_type=AMP_LINK_SEARCH_MODULES_LIST;
                    dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithFormat:@"%@?request=module&search=",wentry->url];
                }
                break;
            case AMP_LINK_BROWSE:
                if (browse_subMode==AMP_LINK_COMPOSERS) {
                    dbWEB_entries[dbWEB_entries_count].url_type=AMP_LINK_BROWSE_COMPOSERS;
                    dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithFormat:@"%@",wentry->url];
                } else if (browse_subMode==AMP_LINK_GROUPS) {
                    dbWEB_entries[dbWEB_entries_count].url_type=AMP_LINK_BROWSE_GROUPS;
                    dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithFormat:@"%@",wentry->url];
                } else if (browse_subMode==AMP_LINK_MODULES) {
                    dbWEB_entries[dbWEB_entries_count].url_type=AMP_LINK_BROWSE_MODULES;
                    dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithFormat:@"%@",wentry->url];
                }
                break;
            case AMP_LINK_LUCKY:
                if (browse_subMode==AMP_LINK_COMPOSERS) {
                    dbWEB_entries[dbWEB_entries_count].url_type=AMP_LINK_COMPOSERS_LIST;
                    dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithFormat:@"%@",wentry->url];
                }
                break;
        }
        
        
        dbWEB_entries[dbWEB_entries_count].isFile=0;
        
        dbWEB_entries_count++;
        dbWEB_entries_index++;
    }
    //populate entries
}

-(void) fillKeysWithBrowserIndex {
    int dbWEB_entries_index;
    
    entries_noMoreToLoad=true;
    dbWEB_hasFiles=0;
    
    if (entries_noMoreToLoad && mSearch) {
        // in case of search, do not ask DB again => duplicate already found entries & filter them
        [self fillSearchWithEntries];
        return;
    }
    
    if (dbWEB_nb_entries) {
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].label=nil;
            dbWEB_entries_data[i].fullpath=nil;
            dbWEB_entries_data[i].URL=nil;
            dbWEB_entries_data[i].url_type=0;
            dbWEB_entries_data[i].info=nil;
            dbWEB_entries_data[i].img_URL=nil;
            dbWEB_entries_data[i].labelAttr=nil;
            dbWEB_entries_data[i].infoAttr=nil;
        }
        free(dbWEB_entries_data);dbWEB_entries_data=NULL;
        dbWEB_nb_entries=0;
    }
    
    typedef struct {
        NSString *name;
        NSString *url;
    } t_browser_entry;
    NSArray *sortedArray;
    NSMutableArray *tmpArray=[[NSMutableArray alloc] init];
    t_browser_entry *we;
    
    ///////////////////////////////////////////////
    // All entries / letter
    ////////////////////////////////////////////////
    we=(t_browser_entry*)calloc(1,sizeof(t_browser_entry)*27);
    we[0].url=@"https://amp.dascene.net/newresult.php?request=list&search=0-9";
    we[0].name=@"#";
    [tmpArray addObject:[NSValue valueWithPointer:&(we[0])]];
    for (int i=0;i<26;i++) {
        we[i+1].url=[NSString stringWithFormat:@"https://amp.dascene.net/newresult.php?request=list&search=%c",'a'+i];
        we[i+1].name=[NSString stringWithFormat:@"%c",'A'+i];
        [tmpArray addObject:[NSValue valueWithPointer:&(we[i+1])]];
    }
    
        sortedArray=tmpArray;
    ////
    dbWEB_nb_entries=[sortedArray count];
    
    //2nd initialize array to receive entries
    dbWEB_entries_data=(t_WEB_browse_entry *)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    memset(dbWEB_entries_data,0,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    dbWEB_entries_index=0;
        dbWEB_entries_count=0;
        dbWEB_entries=dbWEB_entries_data;
    
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_browser_entry *wentry = (t_browser_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        
        dbWEB_entries[dbWEB_entries_count].label=[[NSString alloc] initWithFormat:@"%@",wentry->name];
        
        dbWEB_entries[dbWEB_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@",wentry->name];
        
        dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithString:wentry->url];
        
        dbWEB_entries[dbWEB_entries_count].isFile=0;
        switch (browse_subMode) {
            case AMP_LINK_BROWSE_COMPOSERS:
                dbWEB_entries[dbWEB_entries_count].url_type=AMP_LINK_COMPOSERS_LIST;
                break;
            case AMP_LINK_BROWSE_GROUPS:
                dbWEB_entries[dbWEB_entries_count].url_type=AMP_LINK_GROUPS_LIST;
                break;
            case AMP_LINK_BROWSE_MODULES:
                dbWEB_entries[dbWEB_entries_count].url_type=AMP_LINK_MODULES_LIST;
                break;
        }
        
        
        dbWEB_entries_count++;
        dbWEB_entries_index++;
    }
    //populate entries
}

-(void) fillKeysWithWEBSourceCompleted:(NSMutableArray*)tmpArray entries_count:(int)we_index entries_data:(t_web_file_entry *)we {
    bool sort_entries=false;
    NSArray *sortedArray;
    int dbWEB_entries_index;
    
    if (sort_entries) {
            sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
                NSString *str1=[((t_web_file_entry*)[obj1 pointerValue])->file_name lastPathComponent];
                NSString *str2=[((t_web_file_entry*)[obj2 pointerValue])->file_name lastPathComponent];
                return [str1 caseInsensitiveCompare:str2];
            }];
    } else {
        sortedArray = tmpArray;
    }
    
    dbWEB_nb_entries=(int)[sortedArray count];
    
    //2nd initialize array to receive entries
    dbWEB_entries_data=(t_WEB_browse_entry *)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    memset(dbWEB_entries_data,0,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    dbWEB_entries_index=0;
        dbWEB_entries_count=0;
        dbWEB_entries=dbWEB_entries_data;
    
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_web_file_entry *wef = (t_web_file_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        
        
        dbWEB_entries[dbWEB_entries_count].label=[[NSString alloc] initWithFormat:@"%@",wef->file_name];
        if (wef->file_nameAttr) dbWEB_entries[dbWEB_entries_count].labelAttr=[[NSAttributedString alloc] initWithAttributedString:wef->file_nameAttr];
        
        if (wef->file_URL) dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithString:wef->file_URL];
        
        if (wef->file_img_URL && ([wef->file_img_URL characterAtIndex:[wef->file_img_URL length]-1]!='/') ) dbWEB_entries[dbWEB_entries_count].img_URL=[NSString stringWithString:wef->file_img_URL];
        
        if (wef->url_type==AMP_LINK_MODULE_FILE) {
            dbWEB_entries[dbWEB_entries_count].fullpath=[NSString stringWithFormat:@"Documents/AMP/%@/%@.gz",wef->composer,wef->file_name];
            dbWEB_entries[dbWEB_entries_count].isFile=1;
            dbWEB_entries[dbWEB_entries_count].info=wef->file_details;
        } else if ( (browse_subMode==AMP_LINK_COMPOSERS_LIST)||(browse_subMode==AMP_LINK_SEARCH_COMPOSERS_LIST)) {
            dbWEB_entries[dbWEB_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@",wef->file_name];
            dbWEB_entries[dbWEB_entries_count].isFile=0;
            dbWEB_entries[dbWEB_entries_count].info=wef->file_details;
        } else {
            dbWEB_entries[dbWEB_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@",wef->file_name];
            dbWEB_entries[dbWEB_entries_count].isFile=0;
        }
        
        dbWEB_entries[dbWEB_entries_count].url_type=wef->url_type;
        dbWEB_entries[dbWEB_entries_count].downloaded=-1;
        dbWEB_entries[dbWEB_entries_count].entries_nb=wef->entries_nb;
        
        dbWEB_entries[dbWEB_entries_count].rating=-1;
        dbWEB_entries[dbWEB_entries_count].playcount=-1;
        dbWEB_entries_count++;
        dbWEB_entries_index++;
    }
    
    for (int i=0;i<we_index;i++) {
        we[i].file_URL=nil;
        we[i].file_img_URL=nil;
        we[i].file_name=nil;
        we[i].file_nameAttr=nil;
        we[i].file_details=nil;
    }
    
    mdz_safe_free(we);
    
    if (/*!entries_noMoreToLoad &&*/ mSearch) {
        // in case of search, do not ask DB again => duplicate already found entries & filter them
        [self fillSearchWithEntries];
    }
    
    dispatch_async(dispatch_get_main_queue(), ^(void){
        [self fillKeysCompleted];
    });
}

-(void) fillKeysWithWEBSource {
    
    dbWEB_hasFiles=0;
    
    if (entries_noMoreToLoad && mSearch) {
        // in case of search, do not ask DB again => duplicate already found entries & filter them
        [self fillSearchWithEntries];
        return;
    }
    
    if (dbWEB_nb_entries) {
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].label=nil;
            dbWEB_entries_data[i].fullpath=nil;
            dbWEB_entries_data[i].URL=nil;
            dbWEB_entries_data[i].url_type=0;
            dbWEB_entries_data[i].info=nil;
            dbWEB_entries_data[i].img_URL=nil;
            dbWEB_entries_data[i].labelAttr=nil;
            dbWEB_entries_data[i].infoAttr=nil;
        }
        free(dbWEB_entries_data);dbWEB_entries_data=NULL;
        dbWEB_nb_entries=0;
    }
    
    
    
    //Browse page
    //Download html data
    NSURL *url;
    //NSData  *urlData;
    //TFHpple * doc;
    
    NSMutableArray *tmpArray=[[NSMutableArray alloc] init];
    __block t_web_file_entry *we=NULL;
    __block int we_index=0;
    
    if (shouldReload) {
        shouldReload=false;
        arr_current_fetch_position=0;
        [arr_url_handleList removeAllObjects];
        [arr_url_realnameList removeAllObjects];
        [arr_url_countryList removeAllObjects];
        [arr_url_groupsList removeAllObjects];
        [arr_url_groupsLogoList removeAllObjects];
        [arr_url_fileList removeAllObjects];
        [arr_url_composerList removeAllObjects];
        [arr_url_formatList removeAllObjects];
        [arr_url_sizeList removeAllObjects];
    }
    
    if ((browse_subMode==AMP_LINK_GROUPS_LIST)||
               (browse_subMode==AMP_LINK_SEARCH_GROUPS_LIST)){
        ///////////////////////////////////////////////////////////////////////:
        // AMP Composer list
        ///////////////////////////////////////////////////////////////////////:
        dispatch_async(dispatch_get_main_queue(), ^(void){
            [self updateWaitingDetail:[NSString stringWithFormat:@"fetching from %d",self.arr_current_fetch_position]];
        });
        
        if (browse_subMode==AMP_LINK_SEARCH_GROUPS_LIST) {
            if ([mSearchText length]>0) {
                url = [NSURL URLWithString:[NSString stringWithFormat:@"%@%@&position=%d",mWebBaseURL,mSearchText,arr_current_fetch_position]];
            } else {
                url = [NSURL URLWithString:@""];
            }
        } else url = [NSURL URLWithString:[NSString stringWithFormat:@"%@&position=%d",mWebBaseURL,arr_current_fetch_position]];
        
        
        NSURLSession *session = [NSURLSession sharedSession];

        NSURLSessionDataTask *task =
        [session dataTaskWithURL:url
               completionHandler:^(NSData * _Nullable data,
                                   NSURLResponse * _Nullable response,
                                   NSError * _Nullable error)
        {
            if (error) {
                NSLog(@"Erreur réseau : %@", error);
                [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
                return;
            }

            if (!data) {
                NSLog(@"Aucune donnée reçue");
                [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
                return;
            }

            TFHpple *doc = [[TFHpple alloc] initWithHTMLData:data];
            
            NSArray *arr_tmp_url_realnameList=[doc searchWithXPathQuery:@"//text()[contains(normalize-space(.),'Real Name:')]/following::node()[1]"];
            if ([arr_tmp_url_realnameList count]>0) {
                //have found only 1 group and redirected to composers list
                
                mSearch=0;
                
                NSArray *arr_tmp_url_groupName=[doc searchWithXPathQuery:@"//q"];
                
                TFHppleElement *el_title=[arr_tmp_url_groupName objectAtIndex:0];
                dispatch_async(dispatch_get_main_queue(), ^{
                    navbarTitle.text=el_title.content;
                    self.navigationItem.title=navbarTitle.text;
                    [navbarTitle sizeToFit];
                });
                
                NSArray *arr_tmp_url_handleList=[doc searchWithXPathQuery:@"//text()[contains(normalize-space(.),'Handle:')]/following::a[1]"];
                //NSArray *arr_tmp_url_realnameList=[doc searchWithXPathQuery:@"//text()[contains(normalize-space(.),'Real Name:')]/following::node()[1]"];
                NSArray *arr_tmp_url_countryList=[doc searchWithXPathQuery:@"//text()[contains(normalize-space(.),'Country:')]/following::a[1]"];
                NSArray *arr_tmp_url_groupsList=[doc searchWithXPathQuery:@"//td[@class='descript' and normalize-space(.)='Groups:']/following-sibling::td"];
                
                [arr_url_handleList addObjectsFromArray:arr_tmp_url_handleList];
                [arr_url_realnameList addObjectsFromArray:arr_tmp_url_realnameList];
                [arr_url_countryList addObjectsFromArray:arr_tmp_url_countryList];
                [arr_url_groupsList addObjectsFromArray:arr_tmp_url_groupsList];
                
                int currentHandles=(int)[arr_tmp_url_handleList count];
                
                arr_current_fetch_position+=currentHandles;
                if (currentHandles<50) {
                    entries_noMoreToLoad=true;
                }
                
                int total_handles=(int)[arr_url_handleList count];
                int total_realnames=(int)[arr_url_realnameList count];
                int total_countries=(int)[arr_url_countryList count];
                int total_groups=(int)[arr_url_groupsList count];
                if ( (total_handles!=total_realnames) ||
                    (total_handles!=total_realnames) ||
                    (total_handles!=total_realnames) ||
                    (total_handles!=total_realnames) ) {
                    MDZELog("AMP consistency issue: handles %d real names %d countries %d groups %d\n",total_handles,total_realnames,total_countries,total_groups);
                }
                
                if (total_handles) {
                    we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*total_handles);
                    
                    for (int j=0;j<total_handles;j++) {
                        TFHppleElement *el=[arr_url_handleList objectAtIndex:j];
                        we[we_index].file_URL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el objectForKey:@"href"]];
                        
                        //el=[arr_url objectAtIndex:j];
                        we[we_index].file_name=[NSString stringWithFormat:@"%@",el.content];
                        we[we_index].url_type=AMP_LINK_COMPOSER_DETAILS;
                        we[we_index].entries_nb=0;
                        
                        [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                        we_index++;
                    }
                }
            } else {
                dispatch_async(dispatch_get_main_queue(), ^{
                    // mise à jour UI si nécessaire
                    navbarTitle.text=self.title;
                    self.navigationItem.title=navbarTitle.text;
                });
                
                
                NSArray *arr_tmp_url_groupsList=[doc searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[1]/a"];
                NSArray *arr_tmp_url_groupsLogoList=[doc searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[3]"];
                
                [arr_url_groupsList addObjectsFromArray:arr_tmp_url_groupsList];
                [arr_url_groupsLogoList addObjectsFromArray:arr_tmp_url_groupsLogoList];
                
                //int currentGroups=(int)[arr_url_groupsList count];
                
                //arr_current_fetch_position+=currentGroups;
                //if (currentHandles<50) {
                entries_noMoreToLoad=true;
                //}
                
                int total_groups=(int)[arr_url_groupsList count];
                int total_groupsLogo=(int)[arr_url_groupsLogoList count];
                if (total_groups!=total_groupsLogo) {
                    MDZELog("AMP consistency issue: groups %d logos %d\n",total_groups,total_groupsLogo);
                }
                
                if (total_groups) {
                    we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*total_groups);
                    
                    for (int j=0;j<total_groups;j++) {
                        TFHppleElement *el=[arr_url_groupsList objectAtIndex:j];
                        if (el.content && [el.content length]) {
                            NSString *str=[el objectForKey:@"href"];
                            
                            we[we_index].file_URL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el objectForKey:@"href"]];
                            
                            //el=[arr_url objectAtIndex:j];
                            we[we_index].file_name=[NSString stringWithFormat:@"%@",el.content];
                            we[we_index].url_type=AMP_LINK_COMPOSERS_LIST;
                            we[we_index].entries_nb=0;
                            
                            //Logo isn't really useful and not rendering nice in mini
                            //                    el=[arr_url_groupsLogoList objectAtIndex:j];
                            //                    TFHppleElement *el_img=[el firstChildWithTagName:@"img"];
                            //                    if (el_img) {
                            //                        if ([el_img objectForKey:@"src"]) {
                            //                            we[we_index].file_img_URL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el_img objectForKey:@"src"]];
                            //                            MDZILog("found img: %@",we[we_index].file_img_URL);
                            //                        }
                            //                    }
                            
                            [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                            we_index++;
                        }
                    }
                }
            }

            [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
            
            dispatch_async(dispatch_get_main_queue(), ^{
                // mise à jour UI si nécessaire
            });
        }];

        [task resume];
    }
    
    if ((browse_subMode==AMP_LINK_COMPOSERS_LIST)||
        (browse_subMode==AMP_LINK_SEARCH_COMPOSERS_LIST)) {
        ///////////////////////////////////////////////////////////////////////:
        // AMP Composer list
        ///////////////////////////////////////////////////////////////////////:
        dispatch_async(dispatch_get_main_queue(), ^(void){
            [self updateWaitingDetail:[NSString stringWithFormat:@"fetching from %d",self.arr_current_fetch_position]];
        });
        
        if (browse_subMode==AMP_LINK_SEARCH_COMPOSERS_LIST) {
            if ([mSearchText length]>0) {
                url = [NSURL URLWithString:[NSString stringWithFormat:@"%@%@&position=%d",mWebBaseURL,mSearchText,arr_current_fetch_position]];
            } else {
                url = [NSURL URLWithString:@""];
            }
        } else url = [NSURL URLWithString:[NSString stringWithFormat:@"%@&position=%d",mWebBaseURL,arr_current_fetch_position]];
        
        
        NSURLSession *session = [NSURLSession sharedSession];

        NSURLSessionDataTask *task =
        [session dataTaskWithURL:url
               completionHandler:^(NSData * _Nullable data,
                                   NSURLResponse * _Nullable response,
                                   NSError * _Nullable error)
         {
            if (error) {
                NSLog(@"Erreur réseau : %@", error);
                [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
                return;
            }
            
            if (!data) {
                NSLog(@"Aucune donnée reçue");
                [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
                return;
            }
            
            TFHpple *doc       = [[TFHpple alloc] initWithHTMLData:data];
            
            NSArray *arr_tmp_url_handleList=[doc searchWithXPathQuery:@"//text()[contains(normalize-space(.),'Handle:')]/following::a[1]"];
            NSArray *arr_tmp_url_realnameList=[doc searchWithXPathQuery:@"//text()[contains(normalize-space(.),'Real Name:')]/following::node()[1]"];
            NSArray *arr_tmp_url_countryList=[doc searchWithXPathQuery:@"//text()[contains(normalize-space(.),'Country:')]/following::a[1]"];
            NSArray *arr_tmp_url_groupsList=[doc searchWithXPathQuery:@"//td[@class='descript' and normalize-space(.)='Groups:']/following-sibling::td"];
            
            [arr_url_handleList addObjectsFromArray:arr_tmp_url_handleList];
            [arr_url_realnameList addObjectsFromArray:arr_tmp_url_realnameList];
            [arr_url_countryList addObjectsFromArray:arr_tmp_url_countryList];
            [arr_url_groupsList addObjectsFromArray:arr_tmp_url_groupsList];
            
            int currentHandles=(int)[arr_tmp_url_handleList count];
            
            arr_current_fetch_position+=currentHandles;
            if (currentHandles<50) {
                entries_noMoreToLoad=true;
            }
            
            int total_handles=(int)[arr_url_handleList count];
            int total_realnames=(int)[arr_url_realnameList count];
            int total_countries=(int)[arr_url_countryList count];
            int total_groups=(int)[arr_url_groupsList count];
            if ( (total_handles!=total_realnames) ||
                (total_handles!=total_realnames) ||
                (total_handles!=total_realnames) ||
                (total_handles!=total_realnames) ) {
                MDZELog("AMP consistency issue: handles %d real names %d countries %d groups %d\n",total_handles,total_realnames,total_countries,total_groups);
            }
            
            if (total_handles) {
                we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*total_handles);
                
                for (int j=0;j<total_handles;j++) {
                    TFHppleElement *el=[arr_url_handleList objectAtIndex:j];
                    we[we_index].file_URL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el objectForKey:@"href"]];
                    
                    //el=[arr_url objectAtIndex:j];
                    we[we_index].file_name=[NSString stringWithFormat:@"%@",el.content];
                    we[we_index].url_type=AMP_LINK_COMPOSER_DETAILS;
                    we[we_index].entries_nb=0;
                    
                    el=[arr_url_realnameList objectAtIndex:j];
                    we[we_index].file_details=[NSString stringWithFormat:@"%@",el.content];
                    
                    el=[arr_url_groupsList objectAtIndex:j];
                    if (el && [el.content length]) {
                        we[we_index].file_details=[we[we_index].file_details stringByAppendingFormat:@" • %@",[NSString stringWithFormat:@"%@",el.content]];
                    }
                    
                    [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                    we_index++;
                }
            }
            
            [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
        }];
        
        [task resume];
    }
    if (browse_subMode==AMP_LINK_COMPOSER_DETAILS) {
        ///////////////////////////////////////////////////////////////////////:
        // AMP Composer's details
        ///////////////////////////////////////////////////////////////////////:
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        
        NSURLSession *session = [NSURLSession sharedSession];

        NSURLSessionDataTask *task =
        [session dataTaskWithURL:url
               completionHandler:^(NSData * _Nullable data,
                                   NSURLResponse * _Nullable response,
                                   NSError * _Nullable error)
         {
            if (error) {
                NSLog(@"Erreur réseau : %@", error);
                [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
                return;
            }
            
            if (!data) {
                NSLog(@"Aucune donnée reçue");
                [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
                return;
            }
            
            TFHpple *doc       = [[TFHpple alloc] initWithHTMLData:data];
            
            NSArray *arr_url_realName=[doc searchWithXPathQuery:@"//td[@class='descript' and contains(normalize-space(.), 'Real')]/following-sibling::td[1]"];
            NSArray *arr_url_livedIn=[doc searchWithXPathQuery:@"//td[@class='descript' and contains(normalize-space(.), 'Lived')]/following-sibling::td[1]/a"];
            NSArray *arr_url_exHandlesList=[doc searchWithXPathQuery:@"//td[@class='descript' and contains(normalize-space(.), 'Ex.')]/following-sibling::td[1]"];
            NSArray *arr_url_groupsList=[doc searchWithXPathQuery:@"//td[@class='descript' and contains(normalize-space(.), 'member')]/following-sibling::td[1]"];
            NSArray *arr_url_modulesLink=[doc searchWithXPathQuery:@"//td[@class='descript' and contains(normalize-space(.), 'Modules')]/following-sibling::td[1]/a"];
            NSArray *arr_url_interviewLink=[doc searchWithXPathQuery:@"//td[@class='descript' and contains(normalize-space(.), 'Interview')]/following-sibling::td[1]/a"];
            
            int entries_nb=6;
            TFHppleElement *el;
            
            UIColor *topTextCol,*topTextColH,*bottomTextCol;
            UIColor *topTextColData;
            if (darkMode) {
                topTextCol = [UIColor colorWithRed:0.5f green:0.5f blue:1.0f alpha:1.0f];;
                topTextColData = [UIColor colorWithRed:0.8 green:0.8 blue:1.0 alpha:1.0];
                topTextColH = [UIColor colorWithRed:1 green:1 blue:1 alpha:1.0];
                bottomTextCol = [UIColor colorWithRed:0.6 green:0.6 blue:0.6 alpha:1.0];
            } else {
                topTextCol = [UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];;
                topTextColData = [UIColor colorWithRed:0.2 green:0.2 blue:0.5 alpha:1.0];
                topTextColH = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
                bottomTextCol = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
            }
            
            NSDictionary *baseAttributes = @{
                NSForegroundColorAttributeName:topTextCol,
                NSFontAttributeName:[UIFont systemFontOfSize:17 weight:UIFontWeightSemibold],
                NSBackgroundColorAttributeName:[UIColor clearColor]
            };
            NSDictionary *attributesData = @{
                NSForegroundColorAttributeName:topTextColData,
                NSFontAttributeName:[UIFont systemFontOfSize:17 weight:UIFontWeightSemibold],
            };
            NSRange rangeData;
            
            //add entries for each group & country
            el=[arr_url_groupsList objectAtIndex:0];
            entries_nb+=[[el.content componentsSeparatedByString:@","] count];
            entries_nb+=[arr_url_livedIn count];
            
            we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*entries_nb);
            
            if ([arr_url_modulesLink count]>0) {
                el=[arr_url_modulesLink objectAtIndex:0];
                we[we_index].file_name=[NSString stringWithFormat:@"Modules: %@",el.content];
                
                we[we_index].file_nameAttr=[[NSMutableAttributedString alloc] initWithString:we[we_index].file_name attributes:baseAttributes];
                rangeData = NSMakeRange([@"Modules: " length],[el.content length]);
                [we[we_index].file_nameAttr setAttributes:attributesData range:rangeData];
                
                
                we[we_index].file_URL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el objectForKey:@"href"]];
                we[we_index].url_type=AMP_LINK_MODULES_LIST;
                //MDZILog("url of mods: %@",we[we_index].file_URL);
            } else {
                we[we_index].file_name=[NSString stringWithFormat:@"Modules: N/A"];
                we[we_index].file_URL=nil;
            }
            [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index++])]];
            
            el=[arr_url_realName objectAtIndex:0];
            we[we_index].file_name=[NSString stringWithFormat:@"Real name: %@",el.content];
            
            we[we_index].file_nameAttr=[[NSMutableAttributedString alloc] initWithString:we[we_index].file_name attributes:baseAttributes];
            rangeData = NSMakeRange([@"Real name: " length],[el.content length]);
            [we[we_index].file_nameAttr setAttributes:attributesData range:rangeData];
            we[we_index].file_URL=nil;
            [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index++])]];
            
            we[we_index].file_name=[NSString stringWithFormat:@"Lived in:"];
            we[we_index].file_URL=nil;
            [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index++])]];
            
            for (int i=0;i<[arr_url_livedIn count];i++) {
                el=[arr_url_livedIn objectAtIndex:i];
                NSString *strTmp = [[el.raw componentsSeparatedByString:@"title=\""] lastObject];
                strTmp = [[strTmp componentsSeparatedByString:@"\""] firstObject];
                we[we_index].file_name=[NSString stringWithFormat:@" • %@",strTmp];
                
                TFHppleElement *el_img=[el firstChildWithTagName:@"img"];
                if (el_img) {
                    if ([el_img objectForKey:@"src"]) {
                        we[we_index].file_img_URL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el_img objectForKey:@"src"]];
                        //MDZILog("found img: %@",we[we_index].file_img_URL);
                        we[we_index].file_name=[NSString stringWithFormat:@"%@",strTmp];
                    }
                }
                
                we[we_index].file_nameAttr=[[NSMutableAttributedString alloc] initWithString:we[we_index].file_name attributes:baseAttributes];
                rangeData = NSMakeRange(0,[we[we_index].file_name length]);
                [we[we_index].file_nameAttr setAttributes:attributesData range:rangeData];
                we[we_index].file_URL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el objectForKey:@"href"]];;
                we[we_index].url_type=AMP_LINK_COMPOSERS_LIST;
                
                
                
                [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index++])]];
            }
            
            el=[arr_url_exHandlesList objectAtIndex:0];
            we[we_index].file_name=[NSString stringWithFormat:@"Ex.Handles: %@",el.content];
            
            we[we_index].file_nameAttr=[[NSMutableAttributedString alloc] initWithString:we[we_index].file_name attributes:baseAttributes];
            rangeData = NSMakeRange([@"Ex.Handles: " length],[el.content length]);
            [we[we_index].file_nameAttr setAttributes:attributesData range:rangeData];
            
            we[we_index].file_URL=nil;
            [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index++])]];
            
            el=[arr_url_groupsList objectAtIndex:0];
            we[we_index].file_name=[NSString stringWithFormat:@"Was a member of:"];
            we[we_index].file_URL=nil;
            [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index++])]];
            
            //get list of groups and links
            for (TFHppleElement *child in el.children) {
                if ([child objectForKey:@"href"]) {
                    we[we_index].file_name=[NSString stringWithFormat:@" • %@",child.content];
                    
                    we[we_index].file_nameAttr=[[NSMutableAttributedString alloc] initWithString:we[we_index].file_name attributes:baseAttributes];
                    rangeData = NSMakeRange(0,[we[we_index].file_name length]);
                    [we[we_index].file_nameAttr setAttributes:attributesData range:rangeData];
                    
                    we[we_index].file_URL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[child objectForKey:@"href"]];
                    we[we_index].url_type=AMP_LINK_COMPOSERS_LIST;
                    //
                    //MDZILog("url of %@: %@",we[we_index].file_name,we[we_index].file_URL);
                    [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index++])]];
                }
            }
            
            if ([arr_url_interviewLink count]>0) {
                el=[arr_url_interviewLink objectAtIndex:0];
                NSString *str=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el objectForKey:@"href"]];
                if (![str containsString:@"downcount"]) {
                    we[we_index].file_name=[NSString stringWithFormat:@"Interview"];
                    we[we_index].file_URL=str;
                    we[we_index].url_type=AMP_LINK_INTERVIEW;
                    [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index++])]];
                }
            }
            
            entries_noMoreToLoad=true;
            
            [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
        }];
        
        [task resume];
    }
    if ((browse_subMode==AMP_LINK_MODULES_LIST)||
               (browse_subMode==AMP_LINK_SEARCH_MODULES_LIST)) {
        
        ///////////////////////////////////////////////////////////////////////:
        // AMP Modules list
        ///////////////////////////////////////////////////////////////////////:
        
        dispatch_async(dispatch_get_main_queue(), ^(void){
            [self updateWaitingDetail:[NSString stringWithFormat:@"fetching from %d",self.arr_current_fetch_position]];
        });
        
        if (browse_subMode==AMP_LINK_SEARCH_MODULES_LIST) {
            if ([mSearchText length]>0) {
                url = [NSURL URLWithString:[NSString stringWithFormat:@"%@%@&position=%d",mWebBaseURL,mSearchText,arr_current_fetch_position]];
            } else {
                url = [NSURL URLWithString:@""];
            }
        } else url = [NSURL URLWithString:[NSString stringWithFormat:@"%@&position=%d",mWebBaseURL,arr_current_fetch_position]];
        
        NSURLSession *session = [NSURLSession sharedSession];

        NSURLSessionDataTask *task =
        [session dataTaskWithURL:url
               completionHandler:^(NSData * _Nullable data,
                                   NSURLResponse * _Nullable response,
                                   NSError * _Nullable error)
         {
            if (error) {
                NSLog(@"Erreur réseau : %@", error);
                [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
                return;
            }
            
            if (!data) {
                NSLog(@"Aucune donnée reçue");
                [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
                return;
            }
            
            TFHpple *doc       = [[TFHpple alloc] initWithHTMLData:data];
            
            NSArray *arr_tmp_url_fileList=[doc searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[1]/a"];
            NSArray *arr_tmp_url_composerList=[doc searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[2]/a"];
            NSArray *arr_tmp_url_formatList=[doc searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[3]"];
            NSArray *arr_tmp_url_sizeList=[doc searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[4]"];
            
            [arr_url_fileList addObjectsFromArray:arr_tmp_url_fileList];
            [arr_url_composerList addObjectsFromArray:arr_tmp_url_composerList];
            [arr_url_formatList addObjectsFromArray:arr_tmp_url_formatList];
            [arr_url_sizeList addObjectsFromArray:arr_tmp_url_sizeList];
            
            int currentFiles=(int)[arr_url_fileList count];
            
            arr_current_fetch_position+=currentFiles;
            if (currentFiles<50) {
                entries_noMoreToLoad=true;
            }
            
            int total_files=(int)[arr_url_fileList count];
            int total_composers=(int)[arr_url_composerList count];
            
            //keep last items to remove unrelevant entries at beginning
            while (total_composers>total_files) {
                [arr_url_composerList removeObjectAtIndex:0];
                total_composers--;
            }
            
            int total_formats=(int)[arr_url_formatList count];
            int total_sizes=(int)[arr_url_sizeList count];
            if ( (total_files!=total_formats) ||
                (total_files!=total_composers) ||
                (total_files!=total_sizes) ) {
                MDZELog("AMP consistency issue: files %d composers %d formats %d sizes %d\n",total_files,total_composers,total_formats,total_sizes);
            }
            
            if (total_files) {
                we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*total_files);
                
                for (int j=0;j<total_files;j++) {
                    TFHppleElement *el=[arr_url_fileList objectAtIndex:j];
                    TFHppleElement *el_composer=[arr_url_composerList objectAtIndex:j];
                    TFHppleElement *el_format=[arr_url_formatList objectAtIndex:j];
                    TFHppleElement *el_size=[arr_url_sizeList objectAtIndex:j];
                    
                    we[we_index].file_URL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el objectForKey:@"href"]];
                    we[we_index].composer=[NSString stringWithFormat:@"%@",el_composer.content];
                    we[we_index].file_name=[NSString stringWithFormat:@"%@.%@",[el_format.content lowercaseString],el.content];
                    
                    we[we_index].file_details=[NSString stringWithFormat:@"%@",el_size.content];
                    we[we_index].url_type=AMP_LINK_MODULE_FILE;
                    we[we_index].entries_nb=1;
                    
                    [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                    we_index++;
                }
            }
            
            [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
            
        }];
        
        [task resume];
    }
    if (browse_subMode==AMP_LINK_INTERVIEW) {
        
        ///////////////////////////////////////////////////////////////////////:
        // AMP Modules list
        ///////////////////////////////////////////////////////////////////////:
        
        dispatch_async(dispatch_get_main_queue(), ^(void){
            [self updateWaitingDetail:[NSString stringWithFormat:@"fetching interview"]];
        });
        
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        
        NSURLSession *session = [NSURLSession sharedSession];

        NSURLSessionDataTask *task =
        [session dataTaskWithURL:url
               completionHandler:^(NSData * _Nullable data,
                                   NSURLResponse * _Nullable response,
                                   NSError * _Nullable error)
         {
            if (error) {
                NSLog(@"Erreur réseau : %@", error);
                [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
                return;
            }
            
            if (!data) {
                NSLog(@"Aucune donnée reçue");
                [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
                return;
            }
            
            TFHpple *doc       = [[TFHpple alloc] initWithHTMLData:data];
            
            NSArray *arr_interview=[doc searchWithXPathQuery:@"//div[@id='interview']/ul"];
            
            entries_noMoreToLoad=true;
            
            if ([arr_interview count]!=1) {
                MDZELog("AMP consistency issue interview count: %d\n",(int)[arr_interview count]);
            }
            
            if ([arr_interview count]) {
                TFHppleElement *el=[arr_interview objectAtIndex:0];
                
                htmlData=[NSString stringWithFormat:@""
                          "<!DOCTYPE html>"
                          "<html>"
                          "<head>"
                          "<meta charset='UTF-8'><meta name='viewport' content='width=600, initial-scale=1.0, user-scalable=yes, no-shrink=yes'>"
                          "    <style>"
                          "        * {"
                          "            font-family: Verdana, Geneva, Arial, Helvetica, sans-serif;"
                          "        }"
                          ""
                          "        body {"
                          "            color: white;"
                          "            font-size: small;"
                          "-webkit-text-size-adjust: none;"
                          "            background-color: #123456;"
                          "            margin: 0;"
                          "        }"
                          "TT {"
                          "font-family: Courier;"
                          "}"
                          ".small {"
                          "font-family : tahoma, verdana, arial, geneva, sans-serif;"
                          "font-size : 9pt;"
                          "color : #000000;"
                          "text-decoration : none;"
                          "background-color : rgb(200,200,200);"
                          "}"
                          ""
                          "        #interview {"
                          "            text-align: justify;"
                          "            width: 100%%;"
                          "        }"
                          ""
                          "        #interview p {"
                          "            font-size: 9pt;"
                          "        }"
                          ""
                          "        #interview ul {"
                          "            margin: 0;"
                          "            font-size: 9pt;"
                          "        }"
                          ""
                          "        #interview li {"
                          "            margin: 0;"
                          "            font-size: 9pt;"
                          "            color: #fcce04;"
                          "        }"
                          ""
                          "        #interview span {"
                          "            color: #ffffff;"
                          "        }"
                          ""
                          "        #interview h3 {"
                          "            color: #fcce04;"
                          "            text-decoration: underline;"
                          "        }"
                          ""
                          "        #interview h5 {"
                          "            text-align: left;"
                          "            margin: 0;"
                          "            margin-top: 5px;"
                          "        }"
                          "    A:link {"
                          "    text-decoration : none;"
                          "    font-weight : bold;"
                          "    color : #f63;"
                          "    }"
                          "    A:visited {"
                          "    text-decoration : none;"
                          "    font-weight : bold;"
                          "    color : #f63;"
                          "    }"
                          "    A:hover {"
                          "    /*text-decoration : underline; */"
                          "    color : #fc0;"
                          "    }"
                          "    A.offsite {"
                          "    text-decoration : none;"
                          "    font-weight : normal;"
                          "    color : #f63;"
                          "    background : #006;"
                          "    }"
                          "    </style>"
                          "</head>"
                          "<body>"
                          "<div id=\"interview\">"
                          "<p>"
                          "<center><h3>Interview</h3></center>"
                          "</p>"
                          "<br />"
                          "%@"
                          "</body>"
                          "</html>"
                          "",el.raw];
            }
            
            [self fillKeysWithWEBSourceCompleted:tmpArray entries_count:we_index entries_data:we];
            
        }];
        
        [task resume];
    }
}

#pragma mark -
#pragma mark Table view data source

- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"Cell";
    NSString *cellValue;
    NSAttributedString *cellValueAttr;
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
    
    t_WEB_browse_entry *cur_db_entries;
    
    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    int nb_entries=(search_dbWEB?search_dbWEB_nb_entries:dbWEB_nb_entries);
    
    bool has_mini_img=false;
    if (cur_db_entries && nb_entries) has_mini_img=(cur_db_entries[indexPath.row].img_URL?TRUE:FALSE);
    
    if (forceReloadCells) {
        while ([tableView dequeueReusableCellWithIdentifier:CellIdentifier]) {}
        forceReloadCells=false;
    }
    
    UITableViewCell *cell = [tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
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
        topLabel.font = [UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;;
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
//        bottomLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
//                                   ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;;
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
        
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;
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
    bottomLabel.text=@""; //default value
    bottomImageView.image=nil;
    coverImgView.image=nil;
    
    cell.accessoryType = UITableViewCellAccessoryNone;
    
    if (cur_db_entries && nb_entries) {
        
        cellValue=cur_db_entries[indexPath.row].label;
        cellValueAttr=cur_db_entries[indexPath.row].labelAttr;
        int colFactor;
        //update downloaded if needed
        if(cur_db_entries[indexPath.row].downloaded==-1) {
            NSString *pathToCheck=nil;
            
            if (cur_db_entries[indexPath.row].fullpath)
                pathToCheck=[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getAppHomeDirectory],cur_db_entries[indexPath.row].fullpath];
            if (pathToCheck) {
                if ([mFileMngr fileExistsAtPath:pathToCheck]) cur_db_entries[indexPath.row].downloaded=1;
                else cur_db_entries[indexPath.row].downloaded=0;
            } else cur_db_entries[indexPath.row].downloaded=0;
        }
        
        if(cur_db_entries[indexPath.row].downloaded==1) {
            colFactor=1;
        } else colFactor=0;
        
        if (cur_db_entries[indexPath.row].isFile) { //FILE
            if (colFactor==0) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0];
            topLabel.frame= CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth-PRI_SEC_ACTIONS_IMAGE_SIZE-(has_mini_img?35:0),
                                       22);
            if (cur_db_entries[indexPath.row].downloaded==1) {
                if (cur_db_entries[indexPath.row].rating==-1) {
                    DBHelper::getFileStatsDBmod(cur_db_entries[indexPath.row].fullpath,
                                                &cur_db_entries[indexPath.row].playcount,
                                                &cur_db_entries[indexPath.row].rating,
                                                NULL,
                                                &cur_db_entries[indexPath.row].song_length,
                                                &cur_db_entries[indexPath.row].channels_nb,
                                                &cur_db_entries[indexPath.row].songs);
                }
                if (cur_db_entries[indexPath.row].rating>0) bottomImageView.image=[UIImage imageNamed:ratingImg[RATING_IMG(cur_db_entries[indexPath.row].rating)]];
                
                NSString *bottomStr;
                if (cur_db_entries[indexPath.row].song_length>0)
                    bottomStr=[NSString stringWithFormat:@"%02d:%02d",cur_db_entries[indexPath.row].song_length/1000/60,(cur_db_entries[indexPath.row].song_length/1000)%60];
                else bottomStr=@"--:--";
                if (cur_db_entries[indexPath.row].channels_nb)
                    bottomStr=[NSString stringWithFormat:@"%@・%02dch",bottomStr,cur_db_entries[indexPath.row].channels_nb];
                else bottomStr=[NSString stringWithFormat:@"%@・--ch",bottomStr];
                bottomStr=[NSString stringWithFormat:@"%@・Pl:%d",bottomStr,cur_db_entries[indexPath.row].playcount];
                
                bottomLabel.text=[NSString stringWithFormat:@"%@・%@",cur_db_entries[indexPath.row].info,bottomStr];
                
                bottomLabel.frame = CGRectMake((has_mini_img?35:0)+ 1.0 * cell.indentationWidth+20,
                                               22,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth-PRI_SEC_ACTIONS_IMAGE_SIZE-20-(has_mini_img?35:0),
                                               18);
            } else {
                bottomLabel.text=cur_db_entries[indexPath.row].info;
            }
            if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                [actionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateNormal];
                [actionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateHighlighted];
                [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                [actionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
            } else {
                [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateNormal];
                [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateHighlighted];
                [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
            }
            actionView.frame = CGRectMake(tabView.bounds.size.width-2-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,
                                          0,
                                          PRI_SEC_ACTIONS_IMAGE_SIZE,
                                          PRI_SEC_ACTIONS_IMAGE_SIZE);
            actionView.enabled=YES;
            actionView.hidden=NO;
            
            if (cur_db_entries[indexPath.row].img_URL) {
                coverImgView.image = [imagesCache getImageWithURL:cur_db_entries[indexPath.row].img_URL
                                                           prefix:@"AMP_mini"
                                                             size:CGSizeMake(34.0f, 34.0f)
                                                   forUIImageView:coverImgView];
                //coverImgView.contentMode=UIViewContentModeScaleAspectFit;
            }
        } else { // DIR
            bottomLabel.frame = CGRectMake((has_mini_img?35:0)+ 1.0 * cell.indentationWidth,
                                           22,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth-PRI_SEC_ACTIONS_IMAGE_SIZE-(has_mini_img?35:0),
                                           18);
            if (cur_db_entries[indexPath.row].info) {
                bottomLabel.text=[NSString stringWithFormat:@"%@",cur_db_entries[indexPath.row].info];
            } else {
                bottomLabel.text=nil;
            }
            topLabel.frame= CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth- PRI_SEC_ACTIONS_IMAGE_SIZE-(has_mini_img?35:0),
                                       22);
            if (darkMode) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:1.0f alpha:1.0f];
            else topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
            
            if (cur_db_entries[indexPath.row].img_URL) {
                coverImgView.image = [imagesCache getImageWithURL:cur_db_entries[indexPath.row].img_URL
                                                           prefix:@"AMP_mini"
                                                             size:CGSizeMake(34.0f, 34.0f)
                                                   forUIImageView:coverImgView];
                coverImgView.contentMode=UIViewContentModeScaleAspectFit;
            }
            
            if (cur_db_entries[indexPath.row].URL) cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            else cell.accessoryType = UITableViewCellAccessoryNone;
        }
        if (cellValueAttr) topLabel.attributedText = cellValueAttr;
        else topLabel.text = cellValue;
        
        if ([bottomLabel.text length]>0) {
            topLabel.frame= CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth-(has_mini_img?35:0)-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                       22);
            bottomLabel.frame = CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                                           22,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth-(has_mini_img?35:0)-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                           18);
        } else {
            topLabel.frame= CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth-(has_mini_img?35:0)-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                       35);
            bottomLabel.frame = CGRectMake((has_mini_img?35:0)+1.0 * cell.indentationWidth,
                                           40,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth-(has_mini_img?35:0)-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                           0);
        }
        
        if ((indexPath.row==nb_entries-1)&&(entries_noMoreToLoad==false)) {
            dispatch_async(dispatch_get_main_queue(), ^(void){
                [self fillMoreKeys];
            });
        }
    } else {
        topLabel.text = @"";
    }
    
    return cell;
}

#pragma mark -
#pragma mark Table view delegate

- (void)tableView:(UITableView *)tabView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    t_WEB_browse_entry *cur_db_entries;
    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    
    if (cur_db_entries[indexPath.row].isFile) { //FILE
        //File selected, start download is needed
        NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",cur_db_entries[indexPath.row].fullpath];
        mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
        
        if (cur_db_entries[indexPath.row].downloaded==1) {
            if (mClickedPrimAction) {
                NSMutableArray *array_label = [[NSMutableArray alloc] init];
                NSMutableArray *array_path = [[NSMutableArray alloc] init];
                [array_label addObject:cur_db_entries[indexPath.row].label];
                [array_path addObject:cur_db_entries[indexPath.row].fullpath];
                cur_db_entries[indexPath.row].rating=-1;
                [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];

                [tabView reloadData];
                [tableView layoutIfNeeded];
            } else {
                if ([detailViewController add_to_playlist:localPath fileName:cur_db_entries[indexPath.row].label forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)]) {
                    if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];

                    cur_db_entries[indexPath.row].rating=-1;
                    [tabView reloadData];
                    [tableView layoutIfNeeded];
                }
            }
        } else {
            [self checkCreate:[localPath stringByDeletingLastPathComponent]];
            
            [downloadViewController addURLToDownloadList:cur_db_entries[indexPath.row].URL fileName:cur_db_entries[indexPath.row].label filePath:cur_db_entries[indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
        }
    } else if (cur_db_entries[indexPath.row].URL) {
        
        if (cur_db_entries[indexPath.row].url_type!=AMP_LINK_INTERVIEW) {
            [arr_VC_title addObject:self.title];
            [arr_VC_URL addObject:mWebBaseURL];
            [arr_VC_Mode addObject:[NSNumber numberWithInt:browse_subMode]];
            [arr_VC_search addObject:(self.mSearchText?self.mSearchText:@"")];
            
            //set new title
            self.title = cur_db_entries[indexPath.row].fullpath;
            // Set new directory
            browse_depth = browse_depth+1;
            mWebBaseURL=cur_db_entries[indexPath.row].URL;
            if (mWebBaseURL==nil) mWebBaseURL=@"";
            browse_subMode=cur_db_entries[indexPath.row].url_type;
            
            sort_mode=0;
            entries_noMoreToLoad=false;
            shouldReload=false;
            
            shouldFillKeys=1;
            mSearch=0;
            search_dbWEB=0;  //reset to ensure search_dbWEB is not used by default
            
            dbWEB_entries=NULL;
            search_dbWEB_entries=NULL;
            search_dbWEB_entries_count=0;
            
            dbWEB_nb_entries=0;
            search_dbWEB_nb_entries=0;
            
            search_dbWEB_hasFiles=0;
            dbWEB_hasFiles=0;
            
            mSearchText=nil;
            mClickedPrimAction=0;
            sBar.text=mSearchText;

            arr_url_handleList=[NSMutableArray array];
            arr_url_realnameList=[NSMutableArray array];
            arr_url_countryList=[NSMutableArray array];
            arr_url_groupsList=[NSMutableArray array];
            arr_url_groupsLogoList=[NSMutableArray array];
            
            arr_url_fileList=[NSMutableArray array];
            arr_url_composerList=[NSMutableArray array];
            arr_url_formatList=[NSMutableArray array];
            arr_url_sizeList=[NSMutableArray array];
            
            arr_current_fetch_position=0;
            
            htmlData=nil;
            
            [self updateWaitingDetail:@""];
            [self showWaiting];
            [self flushMainLoop];
            
            [self fillMoreKeys];
        } else {
            
            childController = [[RootViewControllerAMPWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            //set new title
            childController.title = cur_db_entries[indexPath.row].fullpath;
            // Set new directory
            ((RootViewControllerAMPWebParser*)childController)->browse_depth = 1;
            ((RootViewControllerAMPWebParser*)childController)->detailViewController=detailViewController;
            ((RootViewControllerAMPWebParser*)childController)->downloadViewController=downloadViewController;
            ((RootViewControllerAMPWebParser*)childController)->mWebBaseURL=cur_db_entries[indexPath.row].URL;
            
            ((RootViewControllerAMPWebParser*)childController)->browse_subMode=cur_db_entries[indexPath.row].url_type;
            
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
            
            // Scroll indicators will be configured in viewWillAppear
            
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
        }
    }
}

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
    //if (mSearchText) [mSearchText release];

    mSearchText=[[NSString alloc] initWithString:searchText];
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    if (mSearch) shouldFillKeys=1;
    search_dbWEB=0;

    if (mSearch) {
        if ((browse_subMode==AMP_LINK_SEARCH_COMPOSERS_LIST)||
            (browse_subMode==AMP_LINK_SEARCH_GROUPS_LIST)||
            (browse_subMode==AMP_LINK_SEARCH_MODULES_LIST)) {
            entries_noMoreToLoad=false;
            shouldReload=true;
        }
    }

    // Cancel previous search timer to debounce
    [self.searchDebounceTimer invalidate];
    self.searchDebounceTimer = nil;

    // Schedule new search after delay
    self.searchDebounceTimer = [NSTimer scheduledTimerWithTimeInterval:0.5
                                                                 target:self
                                                               selector:@selector(fillKeys)
                                                               userInfo:nil
                                                                repeats:NO];
}

@end
