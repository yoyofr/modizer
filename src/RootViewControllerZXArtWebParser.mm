//
//  RootViewControllerZXArtWebParser.mm
//  modizer1
//
//  Created by Yohann Magnien on 19/05/24.
//  Copyright __YoyoFR / Yohann Magnien__ 2024. All rights reserved.
//

#import "RootViewControllerZXArtWebParser.h"

#import "AFNetworking.h"
#import "AFHTTPSessionManager.h"

enum {
    BROWSE_ALL_INDEX,
    BROWSE_ALL_AUTHORS,
    BROWSE_ALL_AUTHORS_SONGS,
    BROWSE_LATEST,
    BROWSE_TOP
};

@implementation RootViewControllerZXArtWebParser

int qsortZXArt_entries_alpha(const void *entryA, const void *entryB) {
    NSString *strA,*strB;
    NSComparisonResult res;
    strA=((t_WEB_browse_entry*)entryA)->label;
    strB=((t_WEB_browse_entry*)entryB)->label;
    res=[strA localizedCaseInsensitiveCompare:strB];
    if (res==NSOrderedAscending) return -1;
    if (res==NSOrderedSame) return 0;
    return 1; //NSOrderedDescending
}

int qsortZXArt_entries_rating_or_entries(const void *entryA, const void *entryB) {
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


- (void)viewDidLoad {
    [super viewDidLoad];
    browse_mode=BROWSE_ALL_INDEX;
    if ([self.title isEqualToString:NSLocalizedString(@"Latest entries",@"")]) {
        browse_mode=BROWSE_LATEST;
    } else if ([self.title isEqualToString:NSLocalizedString(@"Top entries",@"")]) {
        browse_mode=BROWSE_TOP;
    } else {
        if (browse_depth==2) browse_mode=BROWSE_ALL_AUTHORS;
        else if (browse_depth==3) browse_mode=BROWSE_ALL_AUTHORS_SONGS;
    }
    indexTitleMode=0;
    if (browse_mode==BROWSE_ALL_AUTHORS_SONGS) {
        indexTitleMode=1;
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
        {NSLocalizedString(@"Top entries",@""),@"https://zxart.ee/api/export:zxMusic/limit:300/order:votes,desc"},
        {NSLocalizedString(@"Latest entries",@""),@"http://zxart.ee/api/export:zxMusic/limit:300/order:date,desc"},
        {NSLocalizedString(@"All",@""),@""},  //http://zxart.ee/api/export:zxMusic/limit:100/"
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
        
        dbWEB_entries[index][dbWEB_entries_count[index]].img_URL=nil;
        
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
        NSString *file_author;
        NSString *file_format;
        NSString *file_details;
        float file_rating;
        int entries_nb;
        
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
    
    if (browse_mode==BROWSE_ALL_INDEX) {
        we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*27);
        int we_index=0;
        
        for(int i=0;i<27;i++) {
            //here we have the data
            if (i==0) we[we_index].file_name=@"#";
            else we[we_index].file_name=[NSString stringWithFormat:@"%c",i+'A'-1];
            if (i==0) we[we_index].file_URL=@"https://zxart.ee/eng/music/authors/filter/letter:letter20332/";
            else we[we_index].file_URL=[NSString stringWithFormat:@"https://zxart.ee/eng/music/authors/filter/letter:%c/",i+'a'-1];
            we[we_index].file_author=nil;
            we[we_index].file_format=nil;
            we[we_index].file_rating=0;
            we[we_index].file_type=0;
            [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
            we_index++;
        }
        
    } else if (browse_mode==BROWSE_ALL_AUTHORS) {
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
        
        NSArray *arr_url=[doc searchWithXPathQuery:@"//div[contains(@class, 'contentmodule_content')]//tbody/tr/td[2]/a"];
        NSArray *arr_musicrating=[doc searchWithXPathQuery:@"//div[contains(@class, 'contentmodule_content')]//tbody/tr/td[7]"];
        
        we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[arr_url count]);
        
        if (arr_url&&[arr_url count]) {
            
            for (int j=0;j<[arr_url count];j++) {
                TFHppleElement *el=[arr_url objectAtIndex:j];
                
                we[we_index].file_URL=[NSString stringWithFormat:@"%@",[el objectForKey:@"href"]];
                we[we_index].file_name=[NSString stringWithFormat:@"%@",[el text]];
                we[we_index].file_name=[[we[we_index].file_name componentsSeparatedByString:@"_-_"] lastObject];
                
                we[we_index].file_type=2;
                
                we[we_index].file_format=nil;
                
                el=[arr_musicrating objectAtIndex:j];
                we[we_index].file_rating=[[NSString stringWithFormat:@"%@",[el text]] floatValue];
                
                [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                
                we_index++;
            }
        }
    } else if (browse_mode==BROWSE_ALL_AUTHORS_SONGS) {
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        urlData = [NSData dataWithContentsOfURL:url options:NSUTF8StringEncoding error:nil];
        doc       = [[TFHpple alloc] initWithHTMLData:urlData];
        
        NSArray *arr_realauthor=[doc searchWithXPathQuery:@"//table[contains(@class, 'author_details_info')]//tr/td[1]"];
        if ([arr_realauthor count]>=1) {
            if ([[[arr_realauthor objectAtIndex:0] content] containsString:@"Author's profile page"]) {
                NSArray *arr_realauthor2=[doc searchWithXPathQuery:@"//table[contains(@class, 'author_details_info')]//tr/td[2]/a"];
                NSString *newtitle=[NSString stringWithString:[[arr_realauthor2 objectAtIndex:0] text]];
                dispatch_sync(dispatch_get_main_queue(), ^(void){
                    navbarTitle.text=[NSString stringWithString:newtitle];
                });
                
                mWebBaseURL=[[arr_realauthor2 objectAtIndex:0] objectForKey:@"href"];
                url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
                urlData = [NSData dataWithContentsOfURL:url];
                doc       = [[TFHpple alloc] initWithHTMLData:urlData];
            }
            
        }
        
        
        
        
        
        NSArray *arr_musicformat=[doc searchWithXPathQuery:@"//div[contains(@class, 'contentmodule_content')]//tbody/tr/td[4]"];
        
        NSArray *arr_url=[doc searchWithXPathQuery:@"//div[contains(@class, 'contentmodule_content')]//tbody/tr/td[11]/a[1]"];
        
        //XPath doesn't work for some reason, so instead check html page source code directly to get ratings
        NSMutableArray *arr_musicrating=[NSMutableArray arrayWithCapacity:[arr_url count]];
        char *dataptr=(char*)[urlData bytes];
        char subptr[32];
        while (dataptr=strstr(dataptr,"\"votes\":\"")) {
            dataptr+=strlen("\"votes\":\"");
            int i=0;
            while (dataptr[i] && dataptr[i]!='"') i++;
            if (i>31) break;
            strncpy(subptr,dataptr,i);
            float rating=atof(subptr);
            [arr_musicrating addObject:[NSNumber numberWithFloat:rating]];
        }
        
        we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[arr_url count]);
        
        if (arr_url&&[arr_url count]) {
            
            for (int j=0;j<[arr_url count];j++) {
                TFHppleElement *el=[arr_url objectAtIndex:j];
                
                we[we_index].file_URL=[NSString stringWithFormat:@"%@",[el objectForKey:@"href"]];
                NSLog(@"URL: %@",we[we_index].file_URL);
                
                we[we_index].file_name=[[[[we[we_index].file_URL lastPathComponent] componentsSeparatedByString:@":"] lastObject] stringByRemovingPercentEncoding];
                we[we_index].file_name=[[we[we_index].file_name componentsSeparatedByString:@"_-_"] lastObject];
                
                NSLog(@"name: %@",we[we_index].file_name);
                we[we_index].file_type=1;
                we[we_index].file_author=[mWebBaseURL lastPathComponent];
                NSLog(@"author: %@",we[we_index].file_author);
                
                
                
                if (j<[arr_musicformat count]) {
                    el=[arr_musicformat objectAtIndex:j];
                    NSLog(@"format: %@",[el text]);
                    we[we_index].file_format=[NSString stringWithFormat:@"%@",[el text]];
                } else we[we_index].file_format=nil;
                
                if (j<[arr_musicrating count]) {
                    we[we_index].file_rating=[[arr_musicrating objectAtIndex:j] floatValue];
                } else we[we_index].file_rating=0;
                
                [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
                
                we_index++;
            }
        }
    } else if ((browse_mode==BROWSE_LATEST)||(browse_mode==BROWSE_TOP)) {
        ///////////////////////////////////////////////////////////////////////:
        // ZXArt latest
        ///////////////////////////////////////////////////////////////////////:
        url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",mWebBaseURL]];
        
        __block bool op_done=false;
        __block NSDictionary *data;
        
        AFHTTPSessionManager *manager = [AFHTTPSessionManager manager];
        [manager GET:[url absoluteString]
            parameters:nil
            headers:nil
            progress:^(NSProgress * _Nonnull downloadProgress) {
            
            }
            success:^(NSURLSessionDataTask * _Nonnull task, id  _Nullable responseObject) {
                    data = [responseObject objectForKey:@"responseData"];
            data=[data objectForKey:@"zxMusic"];
            op_done=true;
            }
             failure:^(NSURLSessionDataTask * _Nullable task, NSError * _Nonnull error) {
            NSLog(@"ERROR: %@", error);
            op_done=true;
            }];
        
        while (!op_done) {
            [NSThread sleepForTimeInterval:0.1f];
        }
        
        //urlData = [NSData dataWithContentsOfURL:url];
        
        we=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[data count]);
        int we_index=0;
        
        for(NSDictionary *tmpDict in data){
            //here we have the data
            NSString *url=[NSString stringWithString:[tmpDict objectForKey:@"url"]];
            NSString *author=[[url stringByDeletingLastPathComponent] lastPathComponent];
            NSString *filename=[tmpDict objectForKey:@"originalFileName"];
            filename=[filename stringByRemovingPercentEncoding];
            
            NSLog(@"title: %@", [tmpDict objectForKey:@"title"]);
            NSLog(@"url: %@", [tmpDict objectForKey:@"url"]);
            NSLog(@"type: %@", [tmpDict objectForKey:@"type"]);
            NSLog(@"rating: %@", [tmpDict objectForKey:@"rating"]);
            NSLog(@"year: %@", [tmpDict objectForKey:@"year"]);
            NSLog(@"authorIds: %@", [tmpDict objectForKey:@"authorIds"]);
            NSLog(@"author: %@",author);
            NSLog(@"originalFileName: %@", filename);
            NSLog(@"originalUrl: %@", [tmpDict objectForKey:@"originalUrl"]);
            
            we[we_index].file_name=[NSString stringWithString:filename];
            we[we_index].file_name=[[we[we_index].file_name componentsSeparatedByString:@"_-_"] lastObject];
            we[we_index].file_URL=[NSString stringWithString:[tmpDict objectForKey:@"originalUrl"]];
            we[we_index].file_author=[NSString stringWithString:author];
            we[we_index].file_format=[NSString stringWithString:[tmpDict objectForKey:@"type"]];
            we[we_index].file_rating=[[NSString stringWithString:[tmpDict objectForKey:@"rating"]] floatValue];
            we[we_index].file_type=1;
            [tmpArray addObject:[NSValue valueWithPointer:&(we[we_index])]];
            we_index++;
        }
    }
    
    if (indexTitleMode) {
        sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
            NSString *str1=[((t_web_file_entry*)[obj1 pointerValue])->file_name lastPathComponent];
            NSString *str2=[((t_web_file_entry*)[obj2 pointerValue])->file_name lastPathComponent];
            return [str1 caseInsensitiveCompare:str2];
        }];
    } else {
        
        if (browse_mode==BROWSE_TOP) {
            sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
                NSNumber *str1=[NSNumber numberWithFloat:((t_web_file_entry*)[obj1 pointerValue])->file_rating];
                NSNumber *str2=[NSNumber numberWithFloat:((t_web_file_entry*)[obj2 pointerValue])->file_rating];
                return [str2 compare:str1];
            }];
        } else {
            sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
                NSString *str1=[((t_web_file_entry*)[obj1 pointerValue])->file_name lastPathComponent];
                NSString *str2=[((t_web_file_entry*)[obj2 pointerValue])->file_name lastPathComponent];
                return [str1 caseInsensitiveCompare:str2];
            }];
        }
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
        
        if (wef->file_type==1) dbWEB_entries[index][dbWEB_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%@",wef->file_name];
        else dbWEB_entries[index][dbWEB_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%@",wef->file_name];
        
        if (wef->file_type==1) dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[NSString stringWithFormat:@"Documents/ZXArt/%@/%@",wef->file_author,wef->file_name];
        else dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@",wef->file_name];
        
        if (wef->file_URL) dbWEB_entries[index][dbWEB_entries_count[index]].URL=[NSString stringWithString:wef->file_URL];
        
        dbWEB_entries[index][dbWEB_entries_count[index]].isFile=(wef->file_type==1);
        dbWEB_entries[index][dbWEB_entries_count[index]].downloaded=-1;
        if (wef->file_type) {
            if (wef->file_format) {
                if (wef->file_author) dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"%@・%@・%.1f",wef->file_author,wef->file_format,wef->file_rating];
                else dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"%@・%.1f",wef->file_format,wef->file_rating];
            }
            else {
                if (wef->file_author) dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"%@・%.1f",wef->file_author,wef->file_rating];
                else dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithFormat:@"%.1f",wef->file_rating];
            }
            dbWEB_entries[index][dbWEB_entries_count[index]].webRating=wef->file_rating;
        }
        else {
            dbWEB_entries[index][dbWEB_entries_count[index]].info=@"";
            dbWEB_entries[index][dbWEB_entries_count[index]].entries_nb=0;
        }
        
        dbWEB_entries[index][dbWEB_entries_count[index]].img_URL=nil;
        
        dbWEB_entries[index][dbWEB_entries_count[index]].rating=-1;
        dbWEB_entries[index][dbWEB_entries_count[index]].playcount=-1;
        dbWEB_entries_count[index]++;
        dbWEB_entries_index++;
    }
    
    for (int i=0;i<we_index;i++) {
        we[i].file_URL=nil;
        we[i].file_name=nil;
        we[i].file_details=nil;
        we[i].file_format=nil;
        we[i].file_details=nil;
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
        //bottomLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
//        ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;;
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
            bottomLabel.frame = CGRectMake((has_mini_img?35:0)+ 1.0 * cell.indentationWidth+20,
                                           22,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-20-(has_mini_img?35:0),
                                           18);
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
        
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    topLabel.text = cellValue;
    
    return cell;
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
            [self checkCreate:[localPath stringByDeletingLastPathComponent]];
            
            
            [downloadViewController addURLToDownloadList:cur_db_entries[section][indexPath.row].URL fileName:cur_db_entries[section][indexPath.row].label filePath:cur_db_entries[section][indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
            
        }
    } else {
        childController = [[RootViewControllerZXArtWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
        //set new title
        childController.title = cur_db_entries[section][indexPath.row].fullpath;
        // Set new directory
        ((RootViewControllerZXArtWebParser*)childController)->browse_depth = browse_depth+1;
        ((RootViewControllerZXArtWebParser*)childController)->detailViewController=detailViewController;
        ((RootViewControllerZXArtWebParser*)childController)->downloadViewController=downloadViewController;
        ((RootViewControllerZXArtWebParser*)childController)->mWebBaseURL=cur_db_entries[section][indexPath.row].URL;
        
        
        childController.view.frame=self.view.frame;
        // And push the window
        [self.navigationController pushViewController:childController animated:YES];
    }
}


@end
