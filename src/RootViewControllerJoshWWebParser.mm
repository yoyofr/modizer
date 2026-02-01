//
//  RootViewControllerJoshWWebParser.mm
//  modizer
//
//  Created by Yohann Magnien on 07/05/21.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//


#define PARSER_TIMEOUT 30 //in seconds

#import "RootViewControllerJoshWWebParser.h"
#import "ModizFileHelper.h"
#import "RadioSource.h"

t_joshw_entry joshw_subsites[]= {
    //computers
    {@"https://pc.joshw.info",@"PC Streamed Music",@"JoshW/PC",@"Computers",TRUE,@[],@"https://www.mobygames.com/game/include_dlc:false/include_nsfw:false/platform:dos/platform:windows/release_status:all/title:%@/sort:moby_score/page:1/"},
    {@"https://cdi.joshw.info/amiga",@"Amiga Music",@"JoshW/Amiga",@"Computers",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=4911"},
    {@"https://fmtowns.joshw.info",@"FM Towns Music",@"JoshW/FMT",@"Computers",TRUE,@[],@"https://www.mobygames.com/game/include_dlc:false/include_nsfw:false/platform:fmtowns/release_status:all/title:%@/sort:moby_score/page:1/"},
    {@"https://s98.joshw.info",@"S98 Music",@"JoshW/S98",@"Computers",TRUE,@[],@"https://www.mobygames.com/game/include_dlc:false/include_nsfw:false/platform:pc88/platform:pc98/platform:sharp-x1/platform:sharp-x68000/release_status:all/title:%@/sort:moby_score/page:1/"},
    {@"https://kss.joshw.info/MSX",@"MSX Music",@"JoshW/MSX",@"Computers",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=4929"},
    //consoles&arcade
    {@"https://nsf.joshw.info",@"NES Music",@"JoshW/NES",@"Consoles & Arcade",TRUE,@[@"zzz_prototypes",@"zzz_unlicensed"],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=7&platform_id[]=4936"},
    {@"https://spc.joshw.info",@"SNES Music",@"JoshW/SNES",@"Consoles & Arcade",TRUE,@[@"zzz_prototypes",@"zzz_unlicensed"],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=6"},
    {@"https://usf.joshw.info",@"Nintendo64 Music",@"JoshW/N64",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=3"},
    {@"https://gcn.joshw.info",@"Gamecube Music",@"JoshW/GC",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=2"},
    {@"https://wii.joshw.info",@"Nintendo Wii Music",@"JoshW/Wii",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=9"},
    {@"https://wiiu.joshw.info",@"Nintendo Wii U Music",@"JoshW/WiiU",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=38"},
    {@"https://kss.joshw.info/Master%20System",@"Master System Music",@"JoshW/SMS",@"Consoles & Arcade",TRUE,@[@"zzz_homebrew"],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=35"},
    {@"https://smd.joshw.info",@"Genesis/SegaCD Music",@"JoshW/SMD",@"Consoles & Arcade",TRUE,@[@"zzz_prototypes",@"zzz_unlicensed"],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=18&platform_id[]=21&platform_id[]=33&platform_id[]=36"},
    {@"https://ssf.joshw.info",@"Saturn Music",@"JoshW/Saturn",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=17"},
    {@"https://dsf.joshw.info",@"Dreamcast Music",@"JoshW/DC",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=16"},
    {@"https://hes.joshw.info",@"PC Engine Music",@"JoshW/PCE",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=34&platform_id[]=4955"},
    {@"https://ncd.joshw.info",@"Neo Geo CD Music",@"JoshW/NEOCD",@"Consoles & Arcade",FALSE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=4956"},
    {@"https://psf.joshw.info",@"PlayStation Music",@"JoshW/PS1",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=10"},
    {@"https://psf2.joshw.info",@"PlayStation 2 Music",@"JoshW/PS2",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=11"},
    {@"https://psf3.joshw.info",@"PlayStation 3 Music",@"JoshW/PS3",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=12"},
    {@"https://psf4.joshw.info",@"PlayStation 4 Music",@"JoshW/PS4",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=4919"},
    {@"https://psf5.joshw.info",@"PlayStation 5 Music",@"JoshW/PS5",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=4980"},
    {@"https://xbox.joshw.info",@"XBox Music",@"JoshW/Xbox",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=14"},
    {@"https://x360.joshw.info",@"XBox360 Music",@"JoshW/X360",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=15"},
    {@"https://3do.joshw.info",@"3DO Music",@"JoshW/3DO",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=25"},
    {@"https://switch.joshw.info",@"Nintendo Switch",@"JoshW/Switch",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=4971"},
    {@"https://cdi.joshw.info/cdi",@"Philips CD-i",@"JoshW/CD-i",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=4917"},
    {@"https://cdi.joshw.info/pgm",@"Arcade PGM",@"JoshW/PGM",@"Consoles & Arcade",FALSE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=23"},
    {@"https://hoot.joshw.info/!MDScene_Arcade_VGM",@"Arcade",@"JoshW/Arcade",@"Consoles & Arcade",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=23&platform_id[]=24"},
    //portables
    {@"https://gbs.joshw.info",@"Game Boy Music",@"JoshW/GB",@"Portables",TRUE,@[@"zzz_unlicensed"],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=4&platform_id[]=41"},
    {@"https://gsf.joshw.info",@"Game Boy Advance Music",@"JoshW/GBA",@"Portables",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=5"},
    {@"https://2sf.joshw.info",@"Nintendo DS Music",@"JoshW/NDS",@"Portables",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=8"},
    {@"https://3sf.joshw.info",@"Nintendo 3DS Music",@"JoshW/3DS",@"Portables",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=4912"},
    {@"https://kss.joshw.info/Game%20Gear",@"Sega Game Gear Music",@"JoshW/SGG",@"Portables",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=20"},
    {@"https://wsr.joshw.info",@"WonderSwan Music",@"JoshW/WS",@"Portables",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=4925&platform_id[]=4926"},
    {@"https://psp.joshw.info",@"PSP Music",@"JoshW/PSP",@"Portables",TRUE,@[@"zzz_others"],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=13"},
    {@"https://vita.joshw.info",@"PSVita Music",@"JoshW/PSVita",@"Portables",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=39"},
    {@"https://mobile.joshw.info",@"Mobile/Smartphone Music",@"JoshW/Mobile",@"Portables",TRUE,@[],@"https://thegamesdb.net/search.php?name=%@&platform_id[]=4915&platform_id[]=4916"},
};
int joshw_subsites_size=sizeof(joshw_subsites)/sizeof(t_joshw_entry);

@implementation RootViewControllerJoshWWebParser

@synthesize mWebBaseDir;

#import "AlertsCommonFunctions.h"

-(void) pushRadioButton {
    if ([detailViewController.radioSource isActive]&&(detailViewController.radioSource.mRadioSource==RS_COLLECTION_JOSHW)) {
        [detailViewController stop];
        [detailViewController clearQueue];
        [detailViewController.radioSource stop];
        [self updRadioStatus];
    } else {
        [detailViewController.radioSource stop];
        [detailViewController clearQueue];
        detailViewController.radioSource.mRadioSource=RS_COLLECTION_JOSHW;
        
        t_WEB_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
        int nb_entries=(search_dbWEB?search_dbWEB_nb_entries:dbWEB_nb_entries);
        
        switch (browse_depth) {
            case 0:
                detailViewController.radioSource.mRadioSource_mode=0;
                [detailViewController.radioSource.mSourceData addObject:@"Rand"];
                break;
            case 1:
                detailViewController.radioSource.mRadioSource_mode=1;
                [detailViewController.radioSource.mSourceData removeAllObjects];
                for (int i=0;i<nb_entries;i++) {
                    [detailViewController.radioSource.mSourceData addObject:[NSString stringWithFormat:@"d:%@",cur_db_entries[i].fullpath]];
                    MDZILog("got: %@",[detailViewController.radioSource.mSourceData lastObject]);
                }
                break;
            case 2:
                detailViewController.radioSource.mRadioSource_mode=2;
                [detailViewController.radioSource.mSourceData removeAllObjects];
                for (int i=0;i<nb_entries;i++) {
                    if (cur_db_entries[i].isFile) {
                        [detailViewController.radioSource.mSourceData addObject:[NSString stringWithFormat:@"f:%@|%@|%@|%@",[cur_db_entries[i].URL stringByRemovingPercentEncoding],[cur_db_entries[i].fullpath substringFromIndex:[cur_db_entries[i].fullpath rangeOfString:@"Documents/JoshW/"].location+[@"Documents/JoshW/" length]],cur_db_entries[i].info,img_grabber_url]];
//                        MDZILog("got: %@",[detailViewController.radioSource.mSourceData lastObject]);
                    } else {
                        MDZILog("got: %@ / %@",cur_db_entries[i].label,cur_db_entries[i].fullpath);
                    }
                }
                break;
            default:
                detailViewController.radioSource.mRadioSource_mode=0;
                break;
        }
        [self showToast:NSLocalizedString(@"Launching Radio", @"") duration:2 nearPoint:radioButton.frame.origin];
        
        dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
            [detailViewController.radioSource activate];
        });
        [self updRadioStatus];
    }
    
}

- (void)viewDidLoad {
    START_PROFILE
    [super viewDidLoad];
    
    _scrapper=[[CoverScrapper alloc] init];
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad || [NSProcessInfo processInfo].isiOSAppOnMac) {
        self.hidesBottomBarWhenPushed = YES;
    }
    
    [radioButton addTarget:self action:@selector(pushRadioButton) forControlEvents:UIControlEventTouchUpInside];
    
    //check if folders exist, create if required
    if (browse_depth>=2&&mWebBaseDir) {
        rootDir=[NSString stringWithFormat:@"%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/Documents/%@",mWebBaseDir]];
        BOOL dirExist = [mFileMngr fileExistsAtPath:rootDir];
        if (!dirExist) {
            [mFileMngr createDirectoryAtPath:rootDir withIntermediateDirectories:TRUE attributes:NULL error:NULL];
        }
    }
    
    END_PROFILE
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [self.updRSTimer invalidate];
    self.updRSTimer = nil;
    self.updRSTimer = [NSTimer scheduledTimerWithTimeInterval:0.3
                                                                 target:self
                                                               selector:@selector(updRadioStatus)
                                                               userInfo:nil
                                                                repeats:YES];
    
    [self updRadioStatus];
    [self updRadioButton];
}

-(void) updRadioButton {
//    if (browse_depth>0) {
//        radioButton.hidden=YES;
//        self.radioButtonWidthConstraint.constant=0;
//        [self.view layoutIfNeeded];
//    } else {
//        radioButton.hidden=NO;
//        self.radioButtonWidthConstraint.constant=44;
//        [self.view layoutIfNeeded];
//    }
}

-(void) updRadioStatus {
    if ([detailViewController.radioSource isActive]&&(detailViewController.radioSource.mRadioSource==RS_COLLECTION_JOSHW)) {
        [radioButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    } else {
        [radioButton setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    }
    [self updRadioButton];
}


-(void) fillKeysCompleted {
    [super fillKeysCompleted];
    fillKeysInProgress=0;
    
    [self hideWaiting];
}


-(void) fillKeys {
    if (fillKeysInProgress) return;
    fillKeysInProgress=1;
    
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    if (mSearch) shouldFillKeys=1;
    search_dbWEB=0;
    
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
    
    if (search_dbWEB_nb_entries) {
            for (int j=0;j<search_dbWEB_entries_count;j++) {
                search_dbWEB_entries[j].label=nil;
                search_dbWEB_entries[j].fullpath=nil;
                search_dbWEB_entries[j].URL=nil;
                search_dbWEB_entries[j].info=nil;
                search_dbWEB_entries[j].img_URL=nil;
                search_dbWEB_entries[j].url_img_grabber=nil;
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
                    search_dbWEB_entries[search_dbWEB_entries_count].isFile=dbWEB_entries[j].isFile;
                    search_dbWEB_entries[search_dbWEB_entries_count].info=dbWEB_entries[j].info;
                    search_dbWEB_entries[search_dbWEB_entries_count].url_img_grabber=dbWEB_entries[j].url_img_grabber;
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
            dbWEB_entries_data[i].info=nil;
            dbWEB_entries_data[i].url_img_grabber=nil;
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
    t_categ_entry categ_entry[]= {
        {@"Computers",@"Amiga,FM Towns,Hoot,MSX,PC,S98"},
        {@"Consoles & Arcade",@"3DO,Arcade,CD-i,DC,GC,Genesis/SegaCD,MS,N64,NeoGeoCD,Nes,PCE,PS1,PS2,PS3,PS4,Saturn,Snes,Switch,Wii,WiiU,Xbox,X360"},
        {@"Portables",@"3DS,GBA,GB,Mobile,NDS,PSP,PSVita,SGG,WSR"}
        
    };
    
    for (int i=0;i<sizeof(categ_entry)/sizeof(t_categ_entry);i++) [tmpArray addObject:[NSValue valueWithPointer:&categ_entry[i]]];
    
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
        dbWEB_entries_count=0;
        dbWEB_entries=dbWEB_entries_data;
    
    char str[1024];
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_categ_entry *wentry = (t_categ_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        snprintf(str,1024,"%s",[wentry->category UTF8String]);
        
        dbWEB_entries[dbWEB_entries_count].label=[[NSString alloc] initWithFormat:@"%s",str];
        
        dbWEB_entries[dbWEB_entries_count].fullpath=[NSString stringWithString:wentry->category];
        
        dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithString:wentry->detail];
        
        dbWEB_entries[dbWEB_entries_count].isFile=0;
        
        dbWEB_entries_count++;
        dbWEB_entries_index++;
    }
}


-(void) fillKeysWithRepoList {
    int dbWEB_entries_index;
    
    if (search_dbWEB_nb_entries) {
            for (int j=0;j<search_dbWEB_entries_count;j++) {
                search_dbWEB_entries[j].label=nil;
                search_dbWEB_entries[j].fullpath=nil;
                search_dbWEB_entries[j].URL=nil;
                search_dbWEB_entries[j].info=nil;
                search_dbWEB_entries[j].img_URL=nil;
                search_dbWEB_entries[j].url_img_grabber=nil;
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
                    search_dbWEB_entries[search_dbWEB_entries_count].isFile=dbWEB_entries[j].isFile;
                    search_dbWEB_entries[search_dbWEB_entries_count].info=dbWEB_entries[j].info;
                    search_dbWEB_entries[search_dbWEB_entries_count].has_letter_index=dbWEB_entries[j].has_letter_index;
                    search_dbWEB_entries[search_dbWEB_entries_count].extra_index=dbWEB_entries[j].extra_index;
                    search_dbWEB_entries[search_dbWEB_entries_count].url_img_grabber=dbWEB_entries[j].url_img_grabber;
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
            dbWEB_entries_data[i].info=nil;
            dbWEB_entries_data[i].url_img_grabber=nil;
        }
        free(dbWEB_entries_data);dbWEB_entries_data=NULL;
        dbWEB_nb_entries=0;
    }
    
    
    NSArray *sortedArray;
    NSMutableArray *tmpArray=[[NSMutableArray alloc] init];
    
    
    for (int i=0;i<sizeof(joshw_subsites)/sizeof(t_joshw_entry);i++) {
        if ([mWebBaseDir isEqualToString:joshw_subsites[i].category]) [tmpArray addObject:[NSValue valueWithPointer:&joshw_subsites[i]]];
    }
    
    sortedArray = [tmpArray sortedArrayUsingComparator:^(id obj1, id obj2) {
        NSString *str1=[((t_joshw_entry*)[obj1 pointerValue])->webSite_name  lastPathComponent];
        NSString *str2=[((t_joshw_entry*)[obj2 pointerValue])->webSite_name lastPathComponent];
        return [str1 caseInsensitiveCompare:str2];
    }];
    ////
    
    dbWEB_nb_entries=[sortedArray count];
    
    //2nd initialize array to receive entries
    dbWEB_entries_data=(t_WEB_browse_entry *)calloc(1,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    memset(dbWEB_entries_data,0,dbWEB_nb_entries*sizeof(t_WEB_browse_entry));
    dbWEB_entries_index=0;
        dbWEB_entries_count=0;
        dbWEB_entries=dbWEB_entries_data;
    
    char str[1024];
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_joshw_entry *wentry = (t_joshw_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        snprintf(str,1024,"%s",[wentry->webSite_name UTF8String]);
        
        dbWEB_entries[dbWEB_entries_count].label=[[NSString alloc] initWithFormat:@"%s",str];
        
        dbWEB_entries[dbWEB_entries_count].fullpath=[NSString stringWithString:wentry->webSite_baseDir];
        
        dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithString:wentry->webSite_URL];
        
        dbWEB_entries[dbWEB_entries_count].url_img_grabber=[NSString stringWithString:wentry->gameDBsearchURL];
        
        dbWEB_entries[dbWEB_entries_count].isFile=0;
        
        dbWEB_entries[dbWEB_entries_count].has_letter_index=wentry->has_letter_index;
        dbWEB_entries[dbWEB_entries_count].extra_index=wentry->extra_index;
        dbWEB_entries[dbWEB_entries_count].url_img_grabber=wentry->gameDBsearchURL;
        
        
        dbWEB_entries_count++;
        dbWEB_entries_index++;
    }
}

-(void) fillUrlData:(NSData*)data idx:(int)idx max_cnt:(int)max_cnt {
    urlData[idx]=[NSData dataWithData:data];
    data_cnt++;
    dispatch_async(dispatch_get_main_queue(), ^(void){
        [self updateWaitingDetail:[NSString stringWithFormat:@"%d/%d",data_cnt,max_cnt]];
    });
}

-(void) fillKeysWithWEBSource {
    int dbWEB_entries_index;
    
    dbWEB_hasFiles=search_dbWEB_hasFiles=0;
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    if (mSearch) {
        search_dbWEB=1;
        
        if (search_dbWEB_nb_entries) {
                if (search_dbWEB_entries_count)
                for (int j=0;j<search_dbWEB_entries_count;j++) {
                    search_dbWEB_entries[j].label=nil;
                    search_dbWEB_entries[j].fullpath=nil;
                    search_dbWEB_entries[j].URL=nil;
                    search_dbWEB_entries[j].info=nil;
                    search_dbWEB_entries[j].img_URL=nil;
                }
                search_dbWEB_entries=NULL;
            search_dbWEB_nb_entries=0;
            free(search_dbWEB_entries_data);
        }
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
                    search_dbWEB_entries[search_dbWEB_entries_count].info=dbWEB_entries[j].info;
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
            dbWEB_entries_data[i].info=nil;
        }
        free(dbWEB_entries_data);dbWEB_entries_data=NULL;
        dbWEB_nb_entries=0;
    }
    
    typedef struct {
        NSString *file_URL;
        NSString *file_size;
        int idx;
        NSString *img_grabber;
    } t_web_file_entry;
    
    //Browse page
    //Download html data
    NSURL *url;
    TFHpple * doc;
    NSArray *sortedArray;
    NSMutableArray *tmpArray=[[NSMutableArray alloc] init];
    t_web_file_entry *we[27+4];
    int we_nb[27+4];
    
    
    //1st get the data
    int total_url;
    data_cnt=0;
    if (has_letter_index) {
        total_url=27+[extra_index count];
        for (int i=0;i<total_url;i++) {
            we_nb[i]=0;
            if (i==0) url=[NSURL URLWithString:[NSString stringWithFormat:@"%@/0-9/",mWebBaseURL]];
            else if (i<27) url = [NSURL URLWithString:[NSString stringWithFormat:@"%@/%c/",mWebBaseURL,'a'+i-1]];
            else url = [NSURL URLWithString:[NSString stringWithFormat:@"%@/%@/",mWebBaseURL,extra_index[i-27]]];
            
            dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
                NSData *reqData = [NSData dataWithContentsOfURL:url];
                
                dispatch_async(dispatch_get_main_queue(), ^(void){
                    [self fillUrlData:reqData idx:i max_cnt:total_url];
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
                    [self fillUrlData:reqData idx:i max_cnt:total_url];
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
        
        doc       = [[TFHpple alloc] initWithHTMLData:urlData[i]];
        
        NSArray *arr_url=[doc searchWithXPathQuery:@"/html/body/pre//a[position()>5]/@href"];
        NSArray *arr_text=[doc searchWithXPathQuery:@"/html/body/pre//a[position()>5]/following-sibling::text()[1]"];
        if (arr_url&&[arr_url count]) {
            we_nb[i]=(int)[arr_url count];
            we[i]=(t_web_file_entry*)calloc(1,sizeof(t_web_file_entry)*[arr_url count]);
            for (int j=0;j<[arr_url count];j++) {
                TFHppleElement *e_url=[arr_url objectAtIndex:j];
                TFHppleElement *e_text=[arr_text objectAtIndex:j];
                we[i][j].file_URL=[NSString stringWithString:[e_url text]];
                NSArray *arrtmp=[[e_text raw] componentsSeparatedByString:@" "];
                we[i][j].file_size=[NSString stringWithString:[arrtmp objectAtIndex:[arrtmp count]-3]];
                we[i][j].idx=i;
                
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
        dbWEB_entries_count=0;
        dbWEB_entries=dbWEB_entries_data;
    
    char str[1024];
    for (int i=0;i<dbWEB_nb_entries;i++) {
        t_web_file_entry *wef = (t_web_file_entry *)[[sortedArray objectAtIndex:i] pointerValue];
        snprintf(str,1024,"%s",[[wef->file_URL stringByRemovingPercentEncoding] UTF8String]);
        
        int index=wef->idx;;
        //if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
        //if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
        
        dbWEB_entries[dbWEB_entries_count].label=[[NSString alloc] initWithUTF8String:str];
        
        dbWEB_entries[dbWEB_entries_count].fullpath=[NSString stringWithFormat:@"Documents/%@/%@",mWebBaseDir,dbWEB_entries[dbWEB_entries_count].label];
        
        if (has_letter_index) {
            if (index==0) {
                dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithFormat:@"%@/0-9/%@",mWebBaseURL,wef->file_URL];
            } else if (index<27) {
                dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithFormat:@"%@/%c/%@",mWebBaseURL,'a'+index-1,wef->file_URL];
            } else {
                dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithFormat:@"%@/%@/%@",mWebBaseURL,extra_index[index-27],wef->file_URL];
            }
        } else dbWEB_entries[dbWEB_entries_count].URL=[NSString stringWithFormat:@"%@/%@",mWebBaseURL,wef->file_URL];
        
        dbWEB_entries[dbWEB_entries_count].url_img_grabber=wef->img_grabber;
        
        if (str[strlen(str)-1]!='/') dbWEB_entries[dbWEB_entries_count].isFile=1;
        else dbWEB_entries[dbWEB_entries_count].isFile=0;
        dbWEB_entries[dbWEB_entries_count].downloaded=-1;
        dbWEB_entries[dbWEB_entries_count].info=[NSString stringWithString:wef->file_size];
        
        dbWEB_entries[dbWEB_entries_count].rating=-1;
        dbWEB_entries[dbWEB_entries_count].playcount=-1;
        dbWEB_entries_count++;
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

- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"Cell";
    NSString *cellValue;
    const NSInteger TOP_LABEL_TAG = 1001;
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    const NSInteger BOTTOM_IMAGE_TAG = 1003;
    const NSInteger ACT_IMAGE_TAG = 1004;
    const NSInteger SECACT_IMAGE_TAG = 1005;
    const NSInteger COVER_IMAGE_TAG = 1006;
    
    //UILabel *topLabel;
    CBAutoScrollLabel *topLabel;
    UILabel *bottomLabel;
    UIImageView *bottomImageView,*coverImgView;
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
        
        // Configuration de la cellule
        UIBackgroundConfiguration *backgroundConfig = [UIBackgroundConfiguration listGroupedCellConfiguration];
        backgroundConfig.backgroundColor = [UIColor systemGroupedBackgroundColor];
        cell.backgroundConfiguration = backgroundConfig;
        
        //
        // Create the label for the top row of text
        //
//        topLabel = [[UILabel alloc] init];
//        [cell.contentView addSubview:topLabel];
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
//        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
//                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;;
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
        
        coverImgView=[[UIImageView alloc] initWithImage:nil];
        coverImgView.frame= CGRectMake(0,1,34,34);
        coverImgView.contentMode=UIViewContentModeScaleAspectFit;
        coverImgView.tag = COVER_IMAGE_TAG;
        coverImgView.opaque=FALSE;
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
        topLabel = (CBAutoScrollLabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
        bottomImageView = (UIImageView *)[cell viewWithTag:BOTTOM_IMAGE_TAG];
        coverImgView = (UIImageView *)[cell viewWithTag:COVER_IMAGE_TAG];
        actionView = (UIButton *)[cell viewWithTag:ACT_IMAGE_TAG];
        secActionView = (UIButton *)[cell viewWithTag:SECACT_IMAGE_TAG];
        
//        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
//                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
//        
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
//        topLabel.highlightedTextColor = [UIColor colorWithRed:1 green:1 blue:1 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.6 green:0.6 blue:0.6 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
    } else {
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
//        topLabel.highlightedTextColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0];
    }
    
    bottomImageView.image=nil;
    coverImgView.image=nil;
    
    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                               0,
                               tabView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                               22);
    bottomLabel.frame = CGRectMake((bottomImageView.image?16:0)+1.0 * cell.indentationWidth,
                                   22,
                                   -(bottomImageView.image?16:0)+tabView.bounds.size.width -1.0 * cell.indentationWidth-32,
                                   18);
    bottomLabel.text=@""; //default value
    
    
    cell.accessoryType = UITableViewCellAccessoryNone;
    
    t_WEB_browse_entry *cur_db_entries;
    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    
    cellValue=cur_db_entries[indexPath.row].label;
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
        
        if (cur_db_entries[indexPath.row].downloaded==1) {
            if (cur_db_entries[indexPath.row].rating==-1) {
                signed char avg_rating;
                DBHelper::getFileStatsDBmod(cur_db_entries[indexPath.row].fullpath,
                                            &cur_db_entries[indexPath.row].playcount,
                                            &cur_db_entries[indexPath.row].rating,
                                            &avg_rating,
                                            &cur_db_entries[indexPath.row].song_length,
                                            &cur_db_entries[indexPath.row].channels_nb,
                                            &cur_db_entries[indexPath.row].songs);
                if ((cur_db_entries[indexPath.row].rating==0)&&(avg_rating>0))
                    cur_db_entries[indexPath.row].rating=1;
            }
            if (cur_db_entries[indexPath.row].rating>0) bottomImageView.image=[UIImage imageNamed:ratingImg[RATING_IMG(cur_db_entries[indexPath.row].rating)]];
            
            NSString *bottomStr;
            if (cur_db_entries[indexPath.row].song_length>0)
                bottomStr=[NSString stringWithFormat:@"%02d:%02d",cur_db_entries[indexPath.row].song_length/1000/60,(cur_db_entries[indexPath.row].song_length/1000)%60];
            else bottomStr=@"--:--";
            if (cur_db_entries[indexPath.row].channels_nb)
                bottomStr=[NSString stringWithFormat:@"%@|%02dch",bottomStr,cur_db_entries[indexPath.row].channels_nb];
            else bottomStr=[NSString stringWithFormat:@"%@|--ch",bottomStr];
            if (cur_db_entries[indexPath.row].songs) {
                if (cur_db_entries[indexPath.row].songs==1) bottomStr=[NSString stringWithFormat:@"%@|1 song",bottomStr];
                else bottomStr=[NSString stringWithFormat:@"%@|%d songs",bottomStr,abs(cur_db_entries[indexPath.row].songs)];
            }
            else bottomStr=[NSString stringWithFormat:@"%@|- song",bottomStr];
            bottomStr=[NSString stringWithFormat:@"%@|Pl:%d",bottomStr,cur_db_entries[indexPath.row].playcount];
            
            bottomLabel.text=[NSString stringWithFormat:@"%@|%@",cur_db_entries[indexPath.row].info,bottomStr];
            
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
        actionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
        actionView.enabled=YES;
        actionView.hidden=NO;
        
        NSString *imgPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",[[cur_db_entries[indexPath.row].fullpath stringByDeletingPathExtension] stringByAppendingString:@".png"]];
        bool imgExist=false;
        if (![mFileMngr fileExistsAtPath:imgPath]) {
            imgPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",[[cur_db_entries[indexPath.row].fullpath stringByDeletingPathExtension] stringByAppendingString:@".jpg"]];
            if (![mFileMngr fileExistsAtPath:imgPath]) {
                imgPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",[[cur_db_entries[indexPath.row].fullpath stringByDeletingPathExtension] stringByAppendingString:@".jpeg"]];
                if (![mFileMngr fileExistsAtPath:imgPath]) {
                    imgPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",[[cur_db_entries[indexPath.row].fullpath stringByDeletingPathExtension] stringByAppendingString:@".webp"]];
                    if (![mFileMngr fileExistsAtPath:imgPath]) {
                        imgPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",[[cur_db_entries[indexPath.row].fullpath stringByDeletingPathExtension] stringByAppendingString:@".gif"]];
                        if ([mFileMngr fileExistsAtPath:imgPath]) imgExist=true;
                    } else imgExist=true;
                } else imgExist=true;
            } else imgExist=true;
        } else imgExist=true;
        
        if (imgExist && (cur_db_entries[indexPath.row].downloaded==1)) {
            coverImgView.image=[UIImage imageWithContentsOfFile:imgPath];
        }
        
        bottomImageView.frame = CGRectMake((imgExist?35:0) +1.0*cell.indentationWidth,
                                           24,
                                           14,14);
        
        topLabel.frame= CGRectMake((imgExist?35:0) +  1.0 * cell.indentationWidth,
                                   0,
                                   -(imgExist?35:0) + tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                   22);
        bottomLabel.frame = CGRectMake((bottomImageView.image?16:0)+(imgExist?35:0) + 1.0 * cell.indentationWidth,
                                       22,
                                       -(bottomImageView.image?16:0)-(imgExist?35:0) + tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                       18);
        
    } else { // DIR
        bottomLabel.frame = CGRectMake((bottomImageView.image?16:0)+ 1.0 * cell.indentationWidth,
                                       22,
                                       -(bottomImageView.image?16:0)+tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                       18);
        if (cur_db_entries[indexPath.row].URL) bottomLabel.text=cur_db_entries[indexPath.row].URL;
        topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                   0,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                                   22);
        if (darkMode) topLabel.textColor=[UIColor colorWithRed:MDZ_FOLDER_DARK_R green:MDZ_FOLDER_DARK_G blue:MDZ_FOLDER_DARK_B alpha:1.0f];
        else topLabel.textColor=[UIColor colorWithRed:MDZ_FOLDER_LIGHT_R green:MDZ_FOLDER_LIGHT_G blue:MDZ_FOLDER_LIGHT_B alpha:1.0f];
        
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
        t_WEB_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
        
        //delete file
        NSString *fullpath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",cur_db_entries[indexPath.row].fullpath];
        NSError *err;
        DBHelper::deleteStatsFileDB(fullpath);
        cur_db_entries[indexPath.row].downloaded=0;
        cur_db_entries[indexPath.row].img_URL=nil;
        //delete local file
        [mFileMngr removeItemAtPath:fullpath error:&err];
        [ModizFileHelper cleanAllCoversForFile:fullpath];
        
        //ask for a reload/redraw
        [tabView reloadData];
        
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}

- (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    t_WEB_browse_entry *cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    if (cur_db_entries[indexPath.row].downloaded==1) return YES;
    return NO;
}

#pragma mark -
#pragma mark Table view delegate

- (void) primaryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    {
        t_WEB_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
        
        if (cur_db_entries[indexPath.row].isFile) { //FILE
            //File selected, start download is needed
            NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",cur_db_entries[indexPath.row].fullpath];
            
            if (cur_db_entries[indexPath.row].downloaded==1) {
                NSMutableArray *array_label = [[NSMutableArray alloc] init];
                NSMutableArray *array_path = [[NSMutableArray alloc] init];
                [array_label addObject:cur_db_entries[indexPath.row].label];
                [array_path addObject:cur_db_entries[indexPath.row].fullpath];
                cur_db_entries[indexPath.row].rating=-1;
                [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                
                [tableView reloadData];
                [tableView layoutIfNeeded];
            } else {
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                
                if (settings[ONLINE_JOSHW_IMG_GRABBER].detail.mdz_switch.switch_value) {
                    //try to get a cover
                    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
                        //[self getImgfromImgGrabber:cur_db_entries[indexPath.row].label label:cur_db_entries[indexPath.row].label fullpath:cur_db_entries[indexPath.row].fullpath];
                        [self.scrapper getImgfromImgGrabber:img_grabber_url search_label:cur_db_entries[indexPath.row].label label:cur_db_entries[indexPath.row].label fullpath:cur_db_entries[indexPath.row].fullpath completion:^{
                            [detailViewController checkNewCover];
                            [tableView reloadData];
                            if (miniplayerVC) [self updateMiniPlayer];
                        }];
                    });
                }

                
                [downloadViewController addURLToDownloadList:cur_db_entries[indexPath.row].URL fileName:cur_db_entries[indexPath.row].label filePath:cur_db_entries[indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:1];
                
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
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    t_WEB_browse_entry *cur_db_entries;
    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    
    if (cur_db_entries[indexPath.row].isFile) { //FILE
        //File selected, start download is needed
        NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",cur_db_entries[indexPath.row].fullpath];
        mClickedPrimAction=2;
        
        if (cur_db_entries[indexPath.row].downloaded==1) {
            //add to playlist
            [self addToPlaylistSelView:cur_db_entries[indexPath.row].fullpath label:cur_db_entries[indexPath.row].label showNowListening:true];
            
            cur_db_entries[indexPath.row].rating=-1;
            [tableView reloadData];
            [tableView layoutIfNeeded];
        } else {
            [self checkCreate:[localPath stringByDeletingLastPathComponent]];
            
            if (settings[ONLINE_JOSHW_IMG_GRABBER].detail.mdz_switch.switch_value) {
                //try to get a cover
                dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
                    //[self getImgfromImgGrabber:cur_db_entries[indexPath.row].label label:cur_db_entries[indexPath.row].label fullpath:cur_db_entries[indexPath.row].fullpath];
                    [self.scrapper getImgfromImgGrabber:img_grabber_url search_label:cur_db_entries[indexPath.row].label label:cur_db_entries[indexPath.row].label fullpath:cur_db_entries[indexPath.row].fullpath completion:^{
                        [detailViewController checkNewCover];
                        [tableView reloadData];
                        if (miniplayerVC) [self updateMiniPlayer];
                    }];
                });
            }
            
            [downloadViewController addURLToDownloadList:cur_db_entries[indexPath.row].URL fileName:cur_db_entries[indexPath.row].label filePath:cur_db_entries[indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
        }
    }
    [self hideWaiting];
}

- (void)tableView:(UITableView *)tabView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    t_WEB_browse_entry *cur_db_entries;
    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    
    if (cur_db_entries[indexPath.row].isFile) { //FILE
        //File selected, start download is needed
        NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",cur_db_entries[indexPath.row].fullpath];
        mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
        
        MDZILog("URL: %@",cur_db_entries[indexPath.row].URL);
        
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
            } else {
                if ([detailViewController add_to_playlist:localPath fileName:cur_db_entries[indexPath.row].label forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)]) {
                    if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                    
                    cur_db_entries[indexPath.row].rating=-1;
                    [tabView reloadData];
                }
            }
        } else {
            [self checkCreate:[localPath stringByDeletingLastPathComponent]];
            
            if (settings[ONLINE_JOSHW_IMG_GRABBER].detail.mdz_switch.switch_value) {
                //try to get a cover
                dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
                    //[self getImgfromImgGrabber:cur_db_entries[indexPath.row].label label:cur_db_entries[indexPath.row].label fullpath:cur_db_entries[indexPath.row].fullpath];
                    [self.scrapper getImgfromImgGrabber:img_grabber_url search_label:cur_db_entries[indexPath.row].label label:cur_db_entries[indexPath.row].label fullpath:cur_db_entries[indexPath.row].fullpath completion:^{
                        [detailViewController checkNewCover];
                        [tableView reloadData];
                        if (miniplayerVC) [self updateMiniPlayer];
                    }];
                });
            }
            
            [downloadViewController addURLToDownloadList:cur_db_entries[indexPath.row].URL fileName:cur_db_entries[indexPath.row].label filePath:cur_db_entries[indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
            
        }
    } else {
        childController = [[RootViewControllerJoshWWebParser alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
        //set new title
        childController.title = cur_db_entries[indexPath.row].fullpath;
        // Set new directory
        ((RootViewControllerJoshWWebParser*)childController)->browse_depth = browse_depth+1;
        ((RootViewControllerJoshWWebParser*)childController)->has_letter_index = cur_db_entries[indexPath.row].has_letter_index;
        ((RootViewControllerJoshWWebParser*)childController)->img_grabber_url = cur_db_entries[indexPath.row].url_img_grabber;
        ((RootViewControllerJoshWWebParser*)childController)->extra_index = cur_db_entries[indexPath.row].extra_index;
        ((RootViewControllerJoshWWebParser*)childController)->detailViewController=detailViewController;
        ((RootViewControllerJoshWWebParser*)childController)->downloadViewController=downloadViewController;
        ((RootViewControllerJoshWWebParser*)childController)->mWebBaseURL=cur_db_entries[indexPath.row].URL;
        ((RootViewControllerJoshWWebParser*)childController)->mWebBaseDir=cur_db_entries[indexPath.row].fullpath;
        
//        childController.view.frame=self.view.frame;
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
    }
}

-(void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer {
    CGPoint p = [gestureRecognizer locationInView:self.tableView];
    
    NSIndexPath *indexPath = [self.tableView indexPathForRowAtPoint:p];
    if (indexPath != nil) {
        if (gestureRecognizer.state==UIGestureRecognizerStateBegan) {
            t_WEB_browse_entry *cur_db_entries;
            cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
            
            if (cur_db_entries[indexPath.row].isFile && (cur_db_entries[indexPath.row].downloaded==1)) {
                NSString *filePathNoExt=[cur_db_entries[indexPath.row].fullpath stringByDeletingPathExtension];
            
                UIAlertController *alertC = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Cover",@"")
                                                   message:NSLocalizedString(@"Please select an option",@"")
                                                   preferredStyle:UIAlertControllerStyleAlert];
                
                UIAlertAction* downloadAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Download cover",@"") style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction * action) {
                    // Utilisation
                    CGPoint touchPoint = [gestureRecognizer locationInView:self.view];
                    [self showToast:NSLocalizedString(@"Trying to download a cover",@"") duration:2.0 nearPoint:touchPoint];
                    //try to get a cover
                    
                    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
                        //[self getImgfromImgGrabber:cur_db_entries[indexPath.row].label label:cur_db_entries[indexPath.row].label fullpath:cur_db_entries[indexPath.row].fullpath];
                        [self.scrapper getImgfromImgGrabber:img_grabber_url search_label:cur_db_entries[indexPath.row].label label:cur_db_entries[indexPath.row].label fullpath:cur_db_entries[indexPath.row].fullpath completion:^{
                            [detailViewController checkNewCover];
                            [tableView reloadData];
                            if (miniplayerVC) [self updateMiniPlayer];
                        }];
                    });
                    }];
                [alertC addAction:downloadAction];
                
                
                UIAlertAction* deleteAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Delete cover",@"") style:UIAlertActionStyleDestructive
                    handler:^(UIAlertAction * action) {
                    NSString *imgPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",[filePathNoExt stringByAppendingString:@".png"]];
                    [mFileMngr removeItemAtPath:imgPath error:nil];
                    imgPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",[filePathNoExt stringByAppendingString:@".jpg"]];
                    [mFileMngr removeItemAtPath:imgPath error:nil];
                    imgPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",[filePathNoExt stringByAppendingString:@".jpeg"]];
                    [mFileMngr removeItemAtPath:imgPath error:nil];
                    imgPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",[filePathNoExt stringByAppendingString:@".webp"]];
                    [mFileMngr removeItemAtPath:imgPath error:nil];
                    imgPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",[filePathNoExt stringByAppendingString:@".gif"]];
                    [mFileMngr removeItemAtPath:imgPath error:nil];
                    
                    [detailViewController checkNewCover];
                    [tableView reloadData];
                    if (miniplayerVC) [self updateMiniPlayer];
                }];
                    
                    
                [alertC addAction:deleteAction];
                
                UIAlertAction* closeAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Close",@"") style:UIAlertActionStyleCancel
                    handler:^(UIAlertAction * action) {
                }];
                
                [alertC addAction:closeAction];
                
                [self showAlert:alertC];
                
                
            }
        } else {
        }
    }
}


@end
