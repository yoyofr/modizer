//
//  RootViewControllerSNESMWebParser.mm
//  modizer1
//
//  Created by Yohann Magnien on 07/05/21.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//


#import "RootViewControllerSNESMWebParser.h"

enum {
    BROWSE_DEFAULT=0,
    BROWSE_ALL
};

@implementation RootViewControllerSNESMWebParser

int qsortSNESM_entries_alpha(const void *entryA, const void *entryB) {
    NSString *strA,*strB;
    NSComparisonResult res;
    strA=((t_WEB_browse_entry*)entryA)->label;
    strB=((t_WEB_browse_entry*)entryB)->label;
    res=[strA localizedCaseInsensitiveCompare:strB];
    if (res==NSOrderedAscending) return -1;
    if (res==NSOrderedSame) return 0;
    return 1; //NSOrderedDescending
}

int qsortSNESM_entries_rating_or_entries(const void *entryA, const void *entryB) {
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
        qsort(dbWEB_entries_data,dbWEB_nb_entries,sizeof(t_WEB_browse_entry),qsortSNESM_entries_alpha);
        [self fillKeys]; //update search if active
    } else {
        if (sort_mode==1) {
            navbarTitle.text=[NSString stringWithFormat:@"%@ (rating)",self.title];
            self.navigationItem.title=navbarTitle.text;
        } else {
            navbarTitle.text=[NSString stringWithFormat:@"%@ (packs)",self.title];
            self.navigationItem.title=navbarTitle.text;
        }
        qsort(dbWEB_entries_data,dbWEB_nb_entries,sizeof(t_WEB_browse_entry),qsortSNESM_entries_rating_or_entries);
        [self fillKeys]; //update search if active
    }
    
    [tableView reloadData];
}


- (void)viewDidLoad {
    [super viewDidLoad];
    sort_mode=0;
    //set default sort mode
    if ([mWebBaseURL isEqualToString:@"http://snesmusic.org/v2/stats.php"]) sort_mode=1;
    
    
    browse_mode=BROWSE_DEFAULT;
    if ([self.title isEqualToString:NSLocalizedString(@"All",@"")]) {
        browse_mode=BROWSE_ALL;
    }
    
    self.navigationController.delegate = self;
    
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
    dispatch_async(dispatch_get_main_queue(), ^(void){
        [self fillKeysCompleted];
    });
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
                //r.location=NSNotFound;
                //r = [dbWEB_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                //if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
                if ([self searchStringRegExp:mSearchText sourceString:dbWEB_entries[i][j].label]) {
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
        {NSLocalizedString(@"All",@""),@""},
        {@"Top Packs",@"http://snesmusic.org/v2/stats.php"}
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

-(bool) getSNESMDetails:(NSString*)url fullpath:(NSString*)fullpath web_entry:(t_WEB_browse_entry*)we  {
    //get id
    TFHppleElement *el;
    int entry_id=[[[url componentsSeparatedByString:@"="] lastObject] intValue];
    
    NSString *urlEntry=[NSString stringWithFormat:@"http://snesmusic.org/v2/profile.php?profile=set&selected=%d",entry_id];
    
    NSData *urlDataEntry = [NSData dataWithContentsOfURL:[NSURL URLWithString:[NSString stringWithString:urlEntry]]];
    TFHpple *docEntry       = [[TFHpple alloc] initWithHTMLData:urlDataEntry];
    
    //NSArray *arr_entry_name=[docEntry searchWithXPathQuery:@"/html/body/div[@id='contContainer']/h2[1]/text()"];
    NSArray *arr_entry_file=[docEntry searchWithXPathQuery:@"/html/body//a[@class='download']"];
    NSArray *arr_entry_img=[docEntry searchWithXPathQuery:@"/html/body//img[@class='screen']"];
    
    //TFHppleElement *el=[arr_entry_name firstObject];
    //we[we_index].file_name=[NSString stringWithFormat:@"%@",[el raw]];
    
    if (!arr_entry_file) return false;
    if (![arr_entry_file count]) return false;
    
    el=[arr_entry_file firstObject];
    we->URL=[NSString stringWithFormat:@"http://snesmusic.org/v2/%@",[el objectForKey:@"href"]];
    
    el=[arr_entry_img firstObject];
    we->img_URL=[NSString stringWithFormat:@"http://snesmusic.org/v2/%@",[el objectForKey:@"src"]];
    
    NSURLSessionConfiguration *configuration = [NSURLSessionConfiguration defaultSessionConfiguration];
    AFURLSessionManager *manager = [[AFURLSessionManager alloc] initWithSessionConfiguration:configuration];
    manager.responseSerializer = [AFHTTPResponseSerializer serializer];
    
    NSURL *URL = [NSURL URLWithString:we->img_URL];
    NSURLRequest *request = [NSURLRequest requestWithURL:URL];
    
    NSURLSessionDataTask *dataTask = [manager dataTaskWithRequest:request
                                                   uploadProgress:^(NSProgress * _Nonnull uploadProgress) {
    }
                                                 downloadProgress:^(NSProgress * _Nonnull downloadProgress) {        
        
    }
                                                completionHandler:^(NSURLResponse *response, id responseObject, NSError *error) {
        if (error) {
            NSLog(@"Error: %@", error);
        } else {
            NSData *data=responseObject;
            [data writeToFile:[NSString stringWithFormat:@"%@/%@",NSHomeDirectory(),[[fullpath stringByDeletingPathExtension] stringByAppendingString:@".png"]] atomically:NO];
        }
    }];
    
    [dataTask resume];
    
    [manager invalidateSessionCancelingTasks:NO resetSession:NO];
    
    return true;
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
                //r.location=NSNotFound;
                //r = [dbWEB_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                //if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
                if ([self searchStringRegExp:mSearchText sourceString:dbWEB_entries[i][j].label]) {
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
    int we_index=0;
    
    if (browse_mode==BROWSE_ALL) {
        ///////////////////////////////////////////////
        // All entries / letter
        ////////////////////////////////////////////////
        we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*27);
        we[0].file_URL=@"http://snesmusic.org/v2/select.php?view=sets&char=n1-9&limit=0";
        we[0].file_name=@"#";
        we[0].file_type=0;
        [tmpArray addObject:[NSValue valueWithPointer:&(we[0])]];
        for (int i=0;i<26;i++) {
            we[i+1].file_URL=[NSString stringWithFormat:@"http://snesmusic.org/v2/select.php?view=sets&char=%c&limit=0",'A'+i];
            we[i+1].file_name=[NSString stringWithFormat:@"%c",'A'+i];
            we[i+1].file_type=0;
            [tmpArray addObject:[NSValue valueWithPointer:&(we[i+1])]];
        }
    } else if ([mWebBaseURL isEqualToString:@"http://snesmusic.org/v2/stats.php"]) {
        ///////////////////////////////////////////////////////////////////////:
        // SNESM Top 100
        ///////////////////////////////////////////////////////////////////////:
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
        
        NSArray *arr_url=[doc searchWithXPathQuery:@"/html/body//ol[2]//a"];
        //NSArray *arr_system=[doc searchWithXPathQuery:@"/html/body//tr/td[@class='c'][1]"];
        //NSArray *arr_size=[doc searchWithXPathQuery:@"/html/body//tr/td[@class='c'][2]"];
        //NSArray *arr_rating=[doc searchWithXPathQuery:@"/html/body//tr/td[@class='c'][3]"];
        
        we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[arr_url count]);
        
        if (arr_url&&[arr_url count]) {
            
            for (int j=0;j<[arr_url count];j++) {
                TFHppleElement *el=[arr_url objectAtIndex:j];
                
                we[we_index].file_URL=[NSString stringWithFormat:@"http://snesmusic.org/v2/%@",[el objectForKey:@"href"]];
                we[we_index].file_name=[NSString stringWithFormat:@"%@",[el text]];
                we[we_index].file_type=2;
                
                we[we_index].file_rating=100-j;
                
                [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                
                we_index++;
            }
        }
    } else if ([mWebBaseURL containsString:@"http://snesmusic.org/v2/select.php?view=sets&char="]) {
        ///////////////////////////////////////////////////////////////////////:
        // Games - #-A-Z page
        ///////////////////////////////////////////////////////////////////////:
        int pageNb,total_entries;
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
        
        //check page nb
        NSArray *arr_pageNb=[doc searchWithXPathQuery:@"/html/body/div[@id='contContainer']/text()[2]"];
        //mutiple page
        TFHppleElement *el=[arr_pageNb firstObject];
        total_entries=[[[[el raw] componentsSeparatedByString:@" "] lastObject] intValue];
        
        pageNb=(total_entries-1)/30 + 1;
        
        
        we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*total_entries);
        
        NSURLSessionConfiguration *configuration = [NSURLSessionConfiguration defaultSessionConfiguration];
        AFURLSessionManager *manager = [[AFURLSessionManager alloc] initWithSessionConfiguration:configuration];
        manager.responseSerializer = [AFHTTPResponseSerializer serializer];
        manager.completionQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        
        NSMutableArray *sem_arr=nil;
        NSMutableDictionary *urlData_dic=nil;
        
        //download all remaining pages if pages nb>1
        if (pageNb>1) {
            sem_arr=[[NSMutableArray alloc] initWithCapacity:pageNb-1];
            urlData_dic=[[NSMutableDictionary alloc] initWithCapacity:pageNb-1];
            
            for (int i=1;i<pageNb;i++) {
                //build URL
                NSMutableArray *tmparr=[[mWebBaseURL componentsSeparatedByString:@"="] mutableCopy];
                [tmparr removeLastObject];
                NSString *tmpstr=[tmparr componentsJoinedByString:@"="];
                url = [NSURL URLWithString:[NSString stringWithFormat:@"%@=%d",tmpstr,i*30]];
                
                //build semaphore
                dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
                [sem_arr addObject:semaphore];
                
                //prepare request
                NSURLRequest *request = [NSURLRequest requestWithURL:url];
                NSURLSessionDataTask *dataTask = [manager dataTaskWithRequest:request
                                                               uploadProgress:^(NSProgress * _Nonnull uploadProgress) {
                }
                                                             downloadProgress:^(NSProgress * _Nonnull downloadProgress) {
                }
                                                            completionHandler:^(NSURLResponse *response, id responseObject, NSError *error) {
                    if (error) {
                        NSLog(@"Error: %@", error);
                    } else {
                        //urlData_pages=responseObject;
                        [urlData_dic setObject:responseObject forKey:[NSString stringWithFormat:@"data%d",i]];
                        //NSLog(@"%@ %@", response, responseObject);
                    }
                    dispatch_semaphore_signal(semaphore);
                }];
                [dataTask resume];
            }
        }
        
        for (int i=0;i<pageNb;i++) {
            dispatch_async(dispatch_get_main_queue(), ^(void){
                [self updateWaitingDetail:[NSString stringWithFormat:@"%d/%d",i+1,pageNb]];
            });
            
            if (i>0) {
                url = [NSURL URLWithString:[NSString stringWithFormat:@"%@?p=%d",mWebBaseURL,i+1]];
                //urlData = [NSData dataWithContentsOfURL:url];
                
                //ensure thread has finished
                dispatch_semaphore_t semaphore=[sem_arr objectAtIndex:i-1];
                dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, 30 * NSEC_PER_SEC) /*DISPATCH_TIME_FOREVER*/); //30s timeout
                semaphore=nil;
                //get NSData
                urlData=[urlData_dic objectForKey:[NSString stringWithFormat:@"data%d",i]];
                if (urlData) doc       = [[TFHpple alloc] initWithHTMLData:urlData];
                else {
                    doc=nil;
                    NSLog(@"no data for page %d",i);
                }
            }
            
            if (doc){
                NSArray *arr_url=[doc searchWithXPathQuery:@"/html/body/div[@id='contContainer']//a[contains(@href,'profile.php?profile=set')]"];
                NSArray *arr_details=[doc searchWithXPathQuery:@"/html/body/div[@id='contContainer']//img[(@alt='NTSC-J') or (@alt='NTSC') or (@alt='PAL') or (@alt='Beta') or (@alt='SatellaView') or(@alt='SGB') or (@alt='PublicDomain') or (@alt='Bios')]"];
                
                if (arr_url&&[arr_url count]) {
                    
                    for (int j=0;j<[arr_url count];j++) {
                        TFHppleElement *el;
                        bool skip_entry;
                        
                        skip_entry=false;
                        el=[arr_details objectAtIndex:j];
                        if (el) {
                            we[we_index].file_details=[NSString stringWithString:[el objectForKey:@"alt"]];
                            if ([we[we_index].file_details isEqualToString:@"Bios"]) skip_entry=true;
                        }
                        if (!skip_entry) {
                            el=[arr_url objectAtIndex:j];
                            we[we_index].file_URL=[NSString stringWithFormat:@"http://snesmusic.org/v2/%@",[el objectForKey:@"href"]];
                            we[we_index].file_name=[NSString stringWithFormat:@"%@",[el text]];
                            we[we_index].file_type=2;
                            
                            we[we_index].file_rating=0;
                            
                            if (we_index) {
                                //check previous entry
                                if ([we[we_index-1].file_name isEqualToString:we[we_index].file_name]) {
                                    //same name
                                    we[we_index-1].file_details=[NSString stringWithFormat:@"%@・%@",we[we_index-1].file_details,we[we_index].file_details];
                                    we[we_index].file_URL=nil;
                                    we[we_index].file_name=nil;
                                    we[we_index].file_details=nil;
                                    we[we_index].file_type=0;
                                    we[we_index].file_rating=0;
                                } else {
                                    [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                                    we_index++;
                                }
                            } else {
                                [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                                we_index++;
                            }
                        }
                    }
                }
            }
        }
        [manager invalidateSessionCancelingTasks:NO resetSession:NO];
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
        
        if (wef->file_type) dbWEB_entries[index][dbWEB_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%@.rsn",wef->file_name];
        else dbWEB_entries[index][dbWEB_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%@",wef->file_name];
        
        if (wef->file_type) dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[NSString stringWithFormat:@"Documents/SNESM/%@.rsn",wef->file_name];
        else dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@",wef->file_name];
        
        if (wef->file_URL) dbWEB_entries[index][dbWEB_entries_count[index]].URL=[NSString stringWithString:wef->file_URL];
        
        //if (wef->file_img_URL && ([wef->file_img_URL characterAtIndex:[wef->file_img_URL length]-1]!='/') ) dbWEB_entries[index][dbWEB_entries_count[index]].img_URL=[NSString stringWithString:wef->file_img_URL];
        
        
        
        dbWEB_entries[index][dbWEB_entries_count[index]].isFile=wef->file_type;
        dbWEB_entries[index][dbWEB_entries_count[index]].downloaded=-1;
        if (wef->file_type) {
            dbWEB_entries[index][dbWEB_entries_count[index]].img_URL=[NSHomeDirectory() stringByAppendingFormat:@"/%@",[[dbWEB_entries[index][dbWEB_entries_count[index]].fullpath stringByDeletingPathExtension] stringByAppendingString:@".png"]];
            
            if (![[NSFileManager defaultManager] fileExistsAtPath:dbWEB_entries[index][dbWEB_entries_count[index]].img_URL]) dbWEB_entries[index][dbWEB_entries_count[index]].img_URL=nil;
            
            if (wef->file_rating) dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"%d/100",(int)(100-wef->file_rating)];
            dbWEB_entries[index][dbWEB_entries_count[index]].webRating=wef->file_rating;
            
            if (wef->file_details) {
                if (dbWEB_entries[index][dbWEB_entries_count[index]].info) dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"%@・%@",dbWEB_entries[index][dbWEB_entries_count[index]].info,wef->file_details];
                else dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"%@",wef->file_details];
            }
        }
        else {
            //if (wef->entries_nb>1) dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"%d Packs",wef->entries_nb];
            //else dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"1 Pack"];
            dbWEB_entries[index][dbWEB_entries_count[index]].entries_nb=wef->entries_nb;
        }
        
        dbWEB_entries[index][dbWEB_entries_count[index]].rating=-1;
        dbWEB_entries[index][dbWEB_entries_count[index]].playcount=-1;
        dbWEB_entries_count[index]++;
        dbWEB_entries_index++;
    }
    
    for (int i=0;i<we_index;i++) {
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

#pragma mark -
#pragma mark Table view data source

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
        topLabel.font = [UIFont boldSystemFontOfSize:18];
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
                DBHelper::getFileStatsDBmod(cur_db_entries[section][indexPath.row].fullpath,
                                            &cur_db_entries[section][indexPath.row].playcount,
                                            &cur_db_entries[section][indexPath.row].rating,
                                            NULL,
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
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
        } else {
            [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateNormal];
            [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateHighlighted];
            [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
            [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
        }
        actionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,
                                      0,
                                      PRI_SEC_ACTIONS_IMAGE_SIZE,
                                      PRI_SEC_ACTIONS_IMAGE_SIZE);
        actionView.enabled=YES;
        actionView.hidden=NO;
        
        NSString *imgPath=[NSHomeDirectory() stringByAppendingFormat:@"/%@",[[cur_db_entries[section][indexPath.row].fullpath stringByDeletingPathExtension] stringByAppendingString:@".png"]];
        
        if (cur_db_entries[section][indexPath.row].img_URL) {
            coverImgView.image=[UIImage imageWithContentsOfFile:cur_db_entries[section][indexPath.row].img_URL];
            /*coverImgView.image = [imagesCache getImageWithURL:cur_db_entries[section][indexPath.row].img_URL
             prefix:@"SNESM_mini"
             size:CGSizeMake(34.0f, 34.0f)
             forUIImageView:coverImgView];*/
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
            coverImgView.image=[UIImage imageWithContentsOfFile:cur_db_entries[section][indexPath.row].img_URL];
            /*coverImgView.image = [imagesCache getImageWithURL:cur_db_entries[section][indexPath.row].img_URL
             prefix:@"SNESM_mini"
             size:CGSizeMake(34.0f, 34.0f)
             forUIImageView:coverImgView];*/
            coverImgView.contentMode=UIViewContentModeScaleAspectFit;
        }
        
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    topLabel.text = cellValue;
    
    return cell;
}

- (void) primaryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
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
                t_WEB_browse_entry web_entry;
                if ([self getSNESMDetails:cur_db_entries[section][indexPath.row].URL fullpath:cur_db_entries[section][indexPath.row].fullpath web_entry:&web_entry]) {
                    
                    [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                    
                    cur_db_entries[section][indexPath.row].img_URL=[NSHomeDirectory() stringByAppendingFormat:@"/%@",[[cur_db_entries[section][indexPath.row].fullpath stringByDeletingPathExtension] stringByAppendingString:@".png"]];
                    
                    [downloadViewController addURLToDownloadList:web_entry.URL fileName:cur_db_entries[section][indexPath.row].label filePath:cur_db_entries[section][indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:1];
                } else {
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"No file available for this entry",@"")];
                }
                
            }
        }
        
    }
}
- (void) secondaryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
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
            t_WEB_browse_entry web_entry;
            if ([self getSNESMDetails:cur_db_entries[section][indexPath.row].URL fullpath:cur_db_entries[section][indexPath.row].fullpath web_entry:&web_entry]) {
                
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                
                cur_db_entries[section][indexPath.row].img_URL=[NSHomeDirectory() stringByAppendingFormat:@"/%@",[[cur_db_entries[section][indexPath.row].fullpath stringByDeletingPathExtension] stringByAppendingString:@".png"]];
                
                [downloadViewController addURLToDownloadList:web_entry.URL fileName:cur_db_entries[section][indexPath.row].label filePath:cur_db_entries[section][indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
            } else {
                [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"No file available for this entry",@"")];
            }
        }
    }
}

#pragma mark -
#pragma mark Table view delegate

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
            t_WEB_browse_entry web_entry;
            if ([self getSNESMDetails:cur_db_entries[section][indexPath.row].URL fullpath:cur_db_entries[section][indexPath.row].fullpath web_entry:&web_entry]) {
                
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                
                cur_db_entries[section][indexPath.row].img_URL=[NSHomeDirectory() stringByAppendingFormat:@"/%@",[[cur_db_entries[section][indexPath.row].fullpath stringByDeletingPathExtension] stringByAppendingString:@".png"]];
                
                [downloadViewController addURLToDownloadList:web_entry.URL fileName:cur_db_entries[section][indexPath.row].label filePath:cur_db_entries[section][indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
            } else {
                [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"No file available for this entry",@"")];
            }
        }
    } else {
        childController = [[RootViewControllerSNESMWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
        //set new title
        childController.title = cur_db_entries[section][indexPath.row].fullpath;
        // Set new directory
        ((RootViewControllerSNESMWebParser*)childController)->browse_depth = browse_depth+1;
        ((RootViewControllerSNESMWebParser*)childController)->detailViewController=detailViewController;
        ((RootViewControllerSNESMWebParser*)childController)->downloadViewController=downloadViewController;
        ((RootViewControllerSNESMWebParser*)childController)->mWebBaseURL=cur_db_entries[section][indexPath.row].URL;
        
        childController.view.frame=self.view.frame;
        // And push the window
        [self.navigationController pushViewController:childController animated:YES];
    }
}

@end
