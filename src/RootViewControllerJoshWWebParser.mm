//
//  RootViewControllerJoshWWebParser.mm
//  modizer1
//
//  Created by Yohann Magnien on 07/05/21.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#define PARSER_TIMEOUT 30 //in seconds

#import "RootViewControllerJoshWWebParser.h"

@implementation RootViewControllerJoshWWebParser

@synthesize mWebBaseDir;

- (void)viewDidLoad {
    [super viewDidLoad];
    
    //check if folders exist, create if required
    if (browse_depth>=2&&mWebBaseDir) {
        rootDir=[NSString stringWithFormat:@"%@",[NSHomeDirectory() stringByAppendingFormat:@"/Documents/%@",mWebBaseDir]];
        BOOL dirExist = [mFileMngr fileExistsAtPath:rootDir];
        if (!dirExist) {
            [mFileMngr createDirectoryAtPath:rootDir withIntermediateDirectories:TRUE attributes:NULL error:NULL];
        }
    }
    
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

-(void) fillKeys {
    if (shouldFillKeys) {
        shouldFillKeys=0;
        if (browse_depth==0) [self fillKeysWithRepoCateg];
        else if (browse_depth==1) [self fillKeysWithRepoList];
        else [self fillKeysWithWEBSource];
        
    } else { //reset downloaded, rating & playcount flags
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].downloaded=-1;
            dbWEB_entries_data[i].rating=-1;
            dbWEB_entries_data[i].playcount=-1;
        }
        if (mSearch) {
            if (browse_depth==0) [self fillKeysWithRepoCateg];
            else if (browse_depth==1) [self fillKeysWithRepoList];
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
        
        for (int i=0;i<27;i++) {
            search_dbWEB_entries_count[i]=0;
            if (dbWEB_entries_count[i]) search_dbWEB_entries[i]=&(search_dbWEB_entries_data[search_dbWEB_nb_entries]);
            for (int j=0;j<dbWEB_entries_count[i];j++)  {
                //                r.location=NSNotFound;
                //                r = [dbWEB_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                //                if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
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
        }
        free(dbWEB_entries_data);dbWEB_entries_data=NULL;
        dbWEB_nb_entries=0;
    }
    
    typedef struct {
        NSString *category;
        NSString *detail;
    } t_categ_entry;
    NSArray *sortedArray;
    NSMutableArray *tmpArray=[[NSMutableArray alloc] init];
    t_categ_entry webs_entry[]= {
        {@"Computers",@"Amiga,FM Towns,Hoot,MSX,PC,S98"},
        {@"Consoles",@"3DO,CD-i,DC,GC,Genesis/SegaCD,MS,N64,NeoGeoCD,Nes,PCE,PS1,PS2,PS3,PS4,Saturn,Snes,Switch,Wii,WiiU,Xbox,X360"},
        {@"Portables",@"3DS,GBA,GB,Mobile,NDS,PSP,PSVita,SGG,WSR"}
        
    };
    
    for (int i=0;i<sizeof(webs_entry)/sizeof(t_categ_entry);i++) [tmpArray addObject:[NSValue valueWithPointer:&webs_entry[i]]];
    
    sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
        NSString *str1=[((t_categ_entry*)[obj1 pointerValue])->category  lastPathComponent];
        NSString *str2=[((t_categ_entry*)[obj2 pointerValue])->category lastPathComponent];
        return [str1 caseInsensitiveCompare:str2];
    }];
    
    
    ////
    
    dbWEB_nb_entries=[sortedArray count];
    
    //2nd initialize array to receive entries
    dbWEB_entries_data=(t_WEB_browse_entry *)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    memset(dbWEB_entries_data,0,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    dbWEB_entries_index=0;
    for (int i=0;i<27;i++) {
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
        if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
        if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
        //sections are determined 'on the fly' since result set is already sorted
        if (previndex!=index) {
            if (previndex>index) {
                //NSLog(@"********* %s",str);
            } else dbWEB_entries[index]=&(dbWEB_entries_data[dbWEB_entries_index]);
        }
        
        dbWEB_entries[index][dbWEB_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",str];
        
        dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[NSString stringWithString:wentry->category];
        
        dbWEB_entries[index][dbWEB_entries_count[index]].URL=[NSString stringWithString:wentry->detail];
        
        dbWEB_entries[index][dbWEB_entries_count[index]].isFile=0;
        
        dbWEB_entries_count[index]++;
        dbWEB_entries_index++;
    }
}


-(void) fillKeysWithRepoList {
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
        
        for (int i=0;i<27;i++) {
            search_dbWEB_entries_count[i]=0;
            if (dbWEB_entries_count[i]) search_dbWEB_entries[i]=&(search_dbWEB_entries_data[search_dbWEB_nb_entries]);
            for (int j=0;j<dbWEB_entries_count[i];j++)  {
                //                r.location=NSNotFound;
                //                r = [dbWEB_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                //                if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
                if ([self searchStringRegExp:mSearchText sourceString:dbWEB_entries[i][j].label]) {
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].label=dbWEB_entries[i][j].label;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].downloaded=dbWEB_entries[i][j].downloaded;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].rating=dbWEB_entries[i][j].rating;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].playcount=dbWEB_entries[i][j].playcount;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].fullpath=dbWEB_entries[i][j].fullpath;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].URL=dbWEB_entries[i][j].URL;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].isFile=dbWEB_entries[i][j].isFile;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].info=dbWEB_entries[i][j].info;
                    search_dbWEB_entries[i][search_dbWEB_entries_count[i]].has_letter_index=dbWEB_entries[i][j].has_letter_index;
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
        }
        free(dbWEB_entries_data);dbWEB_entries_data=NULL;
        dbWEB_nb_entries=0;
    }
    
    typedef struct {
        NSString *webSite_URL;
        NSString *webSite_name;
        NSString *webSite_baseDir;
        NSString *category;
        bool has_letter_index;
    } t_webSite_entry;
    NSArray *sortedArray;
    NSMutableArray *tmpArray=[[NSMutableArray alloc] init];
    t_webSite_entry webs_entry[]= {
        //computers
        {@"http://pc.joshw.info",@"PC Streamed Music",@"JoshW/PC",@"Computers",TRUE},
        {@"http://cdi.joshw.info/amiga",@"Amiga Music",@"JoshW/Amiga",@"Computers",TRUE},
        {@"http://fmtowns.joshw.info",@"FM Towns Music",@"JoshW/FMT",@"Computers",TRUE},
        {@"http://s98.joshw.info",@"S98 Music",@"JoshW/S98",@"Computers",TRUE},
        {@"http://kss.joshw.info/MSX",@"MSX Music",@"JoshW/MSX",@"Computers",TRUE},
        //consoles
        {@"http://nsf.joshw.info",@"NES Music",@"JoshW/NES",@"Consoles",TRUE},
        {@"http://spc.joshw.info",@"SNES Music",@"JoshW/SNES",@"Consoles",TRUE},
        {@"http://usf.joshw.info",@"Nintendo64 Music",@"JoshW/N64",@"Consoles",TRUE},
        {@"http://gcn.joshw.info",@"Gamecube Music",@"JoshW/GC",@"Consoles",TRUE},
        {@"http://wii.joshw.info",@"Nintendo Wii Music",@"JoshW/Wii",@"Consoles",TRUE},
        {@"http://wiiu.joshw.info",@"Nintendo Wii U Music",@"JoshW/WiiU",@"Consoles",TRUE},
        {@"http://kss.joshw.info/Master%20System",@"Master System Music",@"JoshW/SMS",@"Consoles",TRUE},
        {@"http://smd.joshw.info",@"Genesis/SegaCD Music",@"JoshW/SMD",@"Consoles",TRUE},
        {@"http://ssf.joshw.info",@"Saturn Music",@"JoshW/Saturn",@"Consoles",TRUE},
        {@"http://dsf.joshw.info",@"Dreamcast Music",@"JoshW/DC",@"Consoles",TRUE},
        {@"http://hes.joshw.info",@"PC Engine Music",@"JoshW/PCE",@"Consoles",TRUE},
        {@"http://ncd.joshw.info",@"Neo Geo CD Music",@"JoshW/NEOCD",@"Consoles",TRUE},
        {@"http://psf.joshw.info",@"PlayStation Music",@"JoshW/PS1",@"Consoles",TRUE},
        {@"http://psf2.joshw.info",@"PlayStation 2 Music",@"JoshW/PS2",@"Consoles",TRUE},
        {@"http://psf3.joshw.info",@"PlayStation 3 Music",@"JoshW/PS3",@"Consoles",TRUE},
        {@"http://xbox.joshw.info",@"XBox Music",@"JoshW/Xbox",@"Consoles",TRUE},
        {@"http://x360.joshw.info",@"XBox360 Music",@"JoshW/X360",@"Consoles",TRUE},
        {@"http://3do.joshw.info",@"3DO Music",@"JoshW/3DO",@"Consoles",TRUE},
        {@"http://switch.joshw.info",@"Nintendo Switch",@"JoshW/Switch",@"Consoles",TRUE},
        {@"http://cdi.joshw.info/cdi",@"Philips CD-i",@"JoshW/CD-i",@"Consoles",TRUE},
        {@"http://psf4.joshw.info",@"Playstation 4",@"JoshW/PS4",@"Consoles",TRUE},
        {@"http://psf5.joshw.info",@"Playstation 5",@"JoshW/PS5",@"Consoles",TRUE},
        {@"http://cdi.joshw.info/pgm",@"Arcade PGM",@"JoshW/PGM",@"Consoles",FALSE},
        //portables
        {@"http://gbs.joshw.info",@"Game Boy Music",@"JoshW/GB",@"Portables",TRUE},
        {@"http://gsf.joshw.info",@"Game Boy Advance Music",@"JoshW/GBA",@"Portables",TRUE},
        {@"http://2sf.joshw.info",@"Nintendo DS Music",@"JoshW/NDS",@"Portables",TRUE},
        {@"http://3sf.joshw.info",@"Nintendo 3DS Music",@"JoshW/3DS",@"Portables",TRUE},
        {@"http://kss.joshw.info/Game%20Gear",@"Sega Game Gear Music",@"JoshW/SGG",@"Portables",TRUE},
        {@"http://wsr.joshw.info",@"WonderSwan Music",@"JoshW/WS",@"Portables",TRUE},
        {@"http://psp.joshw.info",@"PSP Music",@"JoshW/PSP",@"Portables",TRUE},
        {@"http://vita.joshw.info",@"PSVita Music",@"JoshW/PSVita",@"Portables",TRUE},
        {@"http://mobile.joshw.info",@"Mobile/Smartphone Music",@"JoshW/Mobile",@"Portables",TRUE}
    };
    
    //NSLog(@"categ: %@",mWebBaseDir);
    for (int i=0;i<sizeof(webs_entry)/sizeof(t_webSite_entry);i++) {
        if ([mWebBaseDir isEqualToString:webs_entry[i].category]) [tmpArray addObject:[NSValue valueWithPointer:&webs_entry[i]]];
    }
    
    sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
        NSString *str1=[((t_webSite_entry*)[obj1 pointerValue])->webSite_name  lastPathComponent];
        NSString *str2=[((t_webSite_entry*)[obj2 pointerValue])->webSite_name lastPathComponent];
        return [str1 caseInsensitiveCompare:str2];
    }];
    ////
    
    dbWEB_nb_entries=[sortedArray count];
    
    //2nd initialize array to receive entries
    dbWEB_entries_data=(t_WEB_browse_entry *)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    memset(dbWEB_entries_data,0,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    dbWEB_entries_index=0;
    for (int i=0;i<27;i++) {
        dbWEB_entries_count[i]=0;
        dbWEB_entries[i]=NULL;
    }
    
    char str[1024];
    index=-1;
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_webSite_entry *wentry = (t_webSite_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        sprintf(str,"%s",[wentry->webSite_name UTF8String]);
        
        previndex=index;
        index=0;
        if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
        if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
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
        
        dbWEB_entries[index][dbWEB_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",str];
        
        dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[NSString stringWithString:wentry->webSite_baseDir];
        
        dbWEB_entries[index][dbWEB_entries_count[index]].URL=[NSString stringWithString:wentry->webSite_URL];
        
        dbWEB_entries[index][dbWEB_entries_count[index]].isFile=0;
        
        dbWEB_entries[index][dbWEB_entries_count[index]].has_letter_index=wentry->has_letter_index;
        
        /*        dbWEB_entries[index][dbWEB_entries_count[index]].downloaded=-1;
         dbWEB_entries[index][dbWEB_entries_count[index]].rating=-1;
         dbWEB_entries[index][dbWEB_entries_count[index]].playcount=-1;*/
        
        dbWEB_entries_count[index]++;
        dbWEB_entries_index++;
    }
}

-(void) fillUrlData:(NSData*)data idx:(int) idx {
    urlData[idx]=[NSData dataWithData:data];
    data_cnt++;
    dispatch_async(dispatch_get_main_queue(), ^(void){
        [self updateWaitingDetail:[NSString stringWithFormat:@"%d/27",data_cnt]];
    });
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
            for (int i=0;i<27;i++) {
                if (search_dbWEB_entries_count[i])
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
        search_dbWEB_entries_data=(t_WEB_browse_entry*)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
        
        for (int i=0;i<27;i++) {
            search_dbWEB_entries_count[i]=0;
            if (dbWEB_entries_count[i]) search_dbWEB_entries[i]=&(search_dbWEB_entries_data[search_dbWEB_nb_entries]);
            for (int j=0;j<dbWEB_entries_count[i];j++)  {
//                r.location=NSNotFound;
//                r = [dbWEB_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
//                if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
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
        }
        free(dbWEB_entries_data);dbWEB_entries_data=NULL;
        dbWEB_nb_entries=0;
    }
    
    typedef struct {
        NSString *file_URL;
        NSString *file_size;
    } t_web_file_entry;
    
    //Browse page
    //Download html data
    NSURL *url;
//    NSData  *urlData[27];
    TFHpple * doc;
    NSArray *sortedArray;
    NSMutableArray *tmpArray=[[NSMutableArray alloc] init];
    t_web_file_entry *we[27];
    int we_nb[27];
    
    
    //1st get the data
    int total_url;
    data_cnt=0;
    if (has_letter_index) {
        total_url=27;
        for (int i=0;i<27;i++) {
            we_nb[i]=0;
            if (i==0) url=[NSURL URLWithString:[NSString stringWithFormat:@"%@/0-9/",mWebBaseURL]];
            else url = [NSURL URLWithString:[NSString stringWithFormat:@"%@/%c/",mWebBaseURL,'a'+i-1]];
            
            dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
                NSData *reqData = [NSData dataWithContentsOfURL:url];
                
                dispatch_async(dispatch_get_main_queue(), ^(void){
                    [self fillUrlData:reqData idx:i];
                });
            });
        }
    } else {
        total_url=1;
        for (int i=0;i<1;i++) {
            we_nb[i]=0;
            url=[NSURL URLWithString:[NSString stringWithFormat:@"%@/",mWebBaseURL]];
            
            dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
                NSData *reqData = [NSData dataWithContentsOfURL:url];
                
                dispatch_async(dispatch_get_main_queue(), ^(void){
                    [self fillUrlData:reqData idx:i];
                });
            });
        }
    }
    
    NSDate *start_date=[NSDate date];
    bool timeout_msg=false;
    
    while (data_cnt<total_url) {
        [NSThread sleepForTimeInterval:0.1f];
        NSDate *now=[NSDate date];
        if ([now timeIntervalSinceDate:start_date]>PARSER_TIMEOUT) {
            //timeout, probably issue on network
            if (!timeout_msg) {
                timeout_msg=true;
                dispatch_async(dispatch_get_main_queue(), ^(void){
                    [self updateWaitingDetail:NSLocalizedString(@"Timeout",@"")];
                });
            }
        }
        if ([now timeIntervalSinceDate:start_date]>(PARSER_TIMEOUT+2)) {
            //timeout, probably issue on network
            return;
        }
    }
    
    for (int i=0;i<total_url;i++) {
        we_nb[i]=0;
        /*dispatch_async(dispatch_get_main_queue(), ^(void){
            [self updateWaitingDetail:[NSString stringWithFormat:@"%d/27",i+1]];
        });*/
        
//        if (i==0) url=[NSURL URLWithString:[NSString stringWithFormat:@"%@/0-9/",mWebBaseURL]];
//        else url = [NSURL URLWithString:[NSString stringWithFormat:@"%@/%c/",mWebBaseURL,'a'+i-1]];
//        
        //urlData = [NSData dataWithContentsOfURL:url];
        
        doc       = [[TFHpple alloc] initWithHTMLData:urlData[i]];
        
        NSArray *arr_url=[doc searchWithXPathQuery:@"/html/body/pre//a[position()>5]/@href"];
        NSArray *arr_text=[doc searchWithXPathQuery:@"/html/body/pre//a[position()>5]/following-sibling::text()[1]"];
        if (arr_url&&[arr_url count]) {
            we_nb[i]=[arr_url count];
            we[i]=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[arr_url count]);
            for (int j=0;j<[arr_url count];j++) {
                TFHppleElement *e_url=[arr_url objectAtIndex:j];
                TFHppleElement *e_text=[arr_text objectAtIndex:j];
                we[i][j].file_URL=[NSString stringWithString:[e_url text]];
                NSArray *arrtmp=[[e_text raw] componentsSeparatedByString:@" "];
                we[i][j].file_size=[NSString stringWithString:[arrtmp objectAtIndex:[arrtmp count]-3]];
                //NSLog(@"fs:%@",we[i][j].file_size);
                [tmpArray addObject:[NSValue valueWithPointer:&(we[i][j])]];
            }
        } else we[i]=NULL;
    }
    
    sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
        NSString *str1=[((t_web_file_entry*)[obj1 pointerValue])->file_URL lastPathComponent];
        NSString *str2=[((t_web_file_entry*)[obj2 pointerValue])->file_URL lastPathComponent];
        return [str1 caseInsensitiveCompare:str2];
    }];
    
    dbWEB_nb_entries=[sortedArray count];
    
    //2nd initialize array to receive entries
    dbWEB_entries_data=(t_WEB_browse_entry *)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    memset(dbWEB_entries_data,0,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    dbWEB_entries_index=0;
    for (int i=0;i<27;i++) {
        dbWEB_entries_count[i]=0;
        dbWEB_entries[i]=NULL;
    }
    
    char str[1024];
    index=-1;
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_web_file_entry *wef = (t_web_file_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        sprintf(str,"%s",[[wef->file_URL stringByRemovingPercentEncoding] UTF8String]);
        
        //NSLog(@"%@",wef->file_size);
        previndex=index;
        index=0;
        if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
        if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
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
        
        dbWEB_entries[index][dbWEB_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",str];
        
        dbWEB_entries[index][dbWEB_entries_count[index]].fullpath=[NSString stringWithFormat:@"Documents/%@/%@",mWebBaseDir,dbWEB_entries[index][dbWEB_entries_count[index]].label];
        
        
        if (has_letter_index) {
            if (index==0) {
                dbWEB_entries[index][dbWEB_entries_count[index]].URL=[NSString stringWithFormat:@"%@/0-9/%@",mWebBaseURL,wef->file_URL];
            } else {
                dbWEB_entries[index][dbWEB_entries_count[index]].URL=[NSString stringWithFormat:@"%@/%c/%@",mWebBaseURL,'a'+index-1,wef->file_URL];
            }
        } else dbWEB_entries[index][dbWEB_entries_count[index]].URL=[NSString stringWithFormat:@"%@/%@",mWebBaseURL,wef->file_URL];
        
        if (str[strlen(str)-1]!='/') dbWEB_entries[index][dbWEB_entries_count[index]].isFile=1;
        else dbWEB_entries[index][dbWEB_entries_count[index]].isFile=0;
        dbWEB_entries[index][dbWEB_entries_count[index]].downloaded=-1;
        dbWEB_entries[index][dbWEB_entries_count[index]].info=[NSString stringWithString:wef->file_size];
        
        dbWEB_entries[index][dbWEB_entries_count[index]].rating=-1;
        dbWEB_entries[index][dbWEB_entries_count[index]].playcount=-1;
        dbWEB_entries_count[index]++;
        dbWEB_entries_index++;
    }
    
    for (int i=0;i<total_url;i++) {
        for (int j=0;j<we_nb[i];j++) {
            we[i][j].file_URL=nil;
            we[i][j].file_size=nil;
        }
        mdz_safe_free(we[i]);
    }
}

#pragma mark -
#pragma mark Table view data source
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    if (mSearch) return nil;
    
    return indexTitles;
}


- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
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
        if (search_dbWEB_hasFiles) return 28+1;
        return 28;
    } else {
        if (dbWEB_hasFiles) return 28+1;
        return 28;
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
    UILabel *topLabel;
    UILabel *bottomLabel;
    UIImageView *bottomImageView;
    UIButton *actionView,*secActionView;
    NSString *nbFiles=NSLocalizedString(@"%d files.",@"");
    NSString *nb1File=NSLocalizedString(@"1 file.",@"");
    
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
        //cell.selectionStyle=UITableViewCellSelectionStyleGray;
    } else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
        bottomImageView = (UIImageView *)[cell viewWithTag:BOTTOM_IMAGE_TAG];
        actionView = (UIButton *)[cell viewWithTag:ACT_IMAGE_TAG];
        secActionView = (UIButton *)[cell viewWithTag:SECACT_IMAGE_TAG];
        
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
        
        bottomLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
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
                               tabView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                               22);
    bottomLabel.frame = CGRectMake(1.0 * cell.indentationWidth,
                                   22,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32,
                                   18);
    bottomLabel.text=@""; //default value
    bottomImageView.image=nil;
    
    cell.accessoryType = UITableViewCellAccessoryNone;
    
    t_WEB_browse_entry **cur_db_entries;
    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    int section = indexPath.section-1;
    
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
        topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                   0,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                   22);
        if (cur_db_entries[section][indexPath.row].downloaded==1) {
            if (cur_db_entries[section][indexPath.row].rating==-1) {
                DBHelper::getFileStatsDBmod([NSString stringWithFormat:@"Documents/%@%@",mWebBaseDir,cur_db_entries[section][indexPath.row].fullpath],
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
                bottomStr=[NSString stringWithFormat:@"%@|%02dch",bottomStr,cur_db_entries[section][indexPath.row].channels_nb];
            else bottomStr=[NSString stringWithFormat:@"%@|--ch",bottomStr];
            if (cur_db_entries[section][indexPath.row].songs) {
                if (cur_db_entries[section][indexPath.row].songs==1) bottomStr=[NSString stringWithFormat:@"%@|1 song",bottomStr];
                else bottomStr=[NSString stringWithFormat:@"%@|%d songs",bottomStr,cur_db_entries[section][indexPath.row].songs];
            }
            else bottomStr=[NSString stringWithFormat:@"%@|- song",bottomStr];
            bottomStr=[NSString stringWithFormat:@"%@|Pl:%d",bottomStr,cur_db_entries[section][indexPath.row].playcount];
            
            bottomLabel.text=[NSString stringWithFormat:@"%@|%@",cur_db_entries[section][indexPath.row].info,bottomStr];
            
            bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+20,
                                           22,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-20,
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
        actionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
        actionView.enabled=YES;
        actionView.hidden=NO;
        
    } else { // DIR
        bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                       22,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                       18);
        if (cur_db_entries[section][indexPath.row].URL) bottomLabel.text=cur_db_entries[section][indexPath.row].URL;
        topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                   0,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                                   22);
        if (darkMode) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:1.0f alpha:1.0f];
        else topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
        
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
    if (search_dbWEB) {
        if (search_dbWEB_hasFiles) section--;
    } else {
        if (dbWEB_hasFiles) section--;
    }
    if (section>=0) {
        if (cur_db_entries[section][indexPath.row].downloaded==1) return YES;
    }
    return NO;
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
        childController = [[RootViewControllerJoshWWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
        //set new title
        childController.title = cur_db_entries[section][indexPath.row].fullpath;
        // Set new directory
        ((RootViewControllerJoshWWebParser*)childController)->browse_depth = browse_depth+1;
        ((RootViewControllerJoshWWebParser*)childController)->has_letter_index = cur_db_entries[section][indexPath.row].has_letter_index;
        ((RootViewControllerJoshWWebParser*)childController)->detailViewController=detailViewController;
        ((RootViewControllerJoshWWebParser*)childController)->downloadViewController=downloadViewController;
        ((RootViewControllerJoshWWebParser*)childController)->mWebBaseURL=cur_db_entries[section][indexPath.row].URL;
        ((RootViewControllerJoshWWebParser*)childController)->mWebBaseDir=cur_db_entries[section][indexPath.row].fullpath;
        
        childController.view.frame=self.view.frame;
        // And push the window
        [self.navigationController pushViewController:childController animated:YES];
    }
}

@end
