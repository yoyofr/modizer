//
//  OnlineCollectionsCommonFunctions.h
//  modizer
//
//  Shared helpers to detect the playlist entries which are missing locally and to
//  re-queue their download from the online collections (cf OnlineViewController.mm).
//
//  Must be included inside an @implementation. The host class needs:
//   - a DownloadViewController *downloadViewController ivar
//   - WaitingViewCommonMethods.h and AlertsCommonFunctions.h
//   - ModizerTypes.h and RootViewControllerJoshWWebParser.h imported (collections tables)
//

#ifndef OnlineCollectionsCommonFunctions_h
#define OnlineCollectionsCommonFunctions_h

//
// Online collections local folders, as created by the OnlineViewController collections browsers.
// Only the collections for which the remote location can be rebuilt from the local path are
// resolvable: the WEB parsed ones (AMP, JoshW, VGMRips, SNESmusic, SMS Power!, ZXArt) build their
// download URL while parsing the web pages, and that URL is not stored anywhere locally.
//
#define MDZ_AMP_BASEDIR @"AMP"
//AMP artist folder on the server does not always match the composer handle shown in the lists,
//so the module download link is looked up through the modules search
//the search is posted, a GET breaks on the titles holding a quote (%27)
#define MDZ_AMP_HOST_URL @"https://amp.dascene.net"
#define MDZ_AMP_SEARCH_URL @"https://amp.dascene.net/newresult.php"
//SNESmusic sets and screenshots are served by set short name
#define MDZ_SNESM_DOWNLOAD_URL @"https://snesmusic.org/v2/download.php?spcNow="
#define MDZ_SNESM_SCREENSHOT_URL @"https://snesmusic.org/v2/images/screenshots/"

//collections whose remote location can be rebuilt from the local path, shown as-is to the user
#define MDZ_RESOLVABLE_COLLECTIONS @"MODLAND, HVSC, ASMA, CGSC, AMP, SNESmusic, JoshW"

//SNESmusic sets list, used to get the download short name out of the local (long) set name
extern const rsn_name_mapping_t SNESmusic_names[];
extern int snes_spc_entries;

//JoshW sub sites list, giving the base URL and the letter bins layout of each system
extern t_joshw_entry joshw_subsites[];
extern int joshw_subsites_size;

-(bool) isResolvableCollectionBaseDir:(NSString*)baseDir {
    return ([baseDir isEqualToString:MODLAND_BASEDIR]||
            [baseDir isEqualToString:HVSC_BASEDIR]||
            [baseDir isEqualToString:ASMA_BASEDIR]||
            [baseDir isEqualToString:CGSC_BASEDIR]||
            [baseDir isEqualToString:MDZ_AMP_BASEDIR]||
            [baseDir isEqualToString:SNESmusic_BASEDIR]||
            [baseDir isEqualToString:JOSHW_BASEDIR]);
}

-(bool) isOnlineCollectionBaseDir:(NSString*)baseDir {
    return ([self isResolvableCollectionBaseDir:baseDir]||
            [baseDir isEqualToString:VGMR_BASEDIR]||
            [baseDir isEqualToString:SMSP_BASEDIR]||
            [baseDir isEqualToString:ZXART_BASEDIR]);
}

//returns the collection folder name (MODLAND, HVSC, ...) of a "Documents/<collection>/..." path, nil otherwise
-(NSString*) onlineCollectionBaseDirForLocalPath:(NSString*)localPath {
    if (localPath==nil) return nil;
    NSArray *comp=[localPath componentsSeparatedByString:@"/"];
    if ([comp count]<3) return nil;
    if (![[comp objectAtIndex:0] isEqualToString:@"Documents"]) return nil;
    NSString *baseDir=[comp objectAtIndex:1];
    if (![self isOnlineCollectionBaseDir:baseDir]) return nil;
    return baseDir;
}

-(bool) isLocalFileAvailable:(NSString*)fullpath {
    //remove the archive index (@) and subsong index (?) suffixes if any
    NSString *localPath=[ModizFileHelper getFullCleanFilePath:fullpath];
    if (localPath==nil) return false;
    NSFileManager *fileMngr=[[NSFileManager alloc] init];
    return [fileMngr fileExistsAtPath:[ModizFileHelper getFullPathForFilePath:localPath]];
}

#pragma mark - Download controller lookup

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

//
// myTabBarController removes the DownloadViewController from its tabs once loaded (it is not a
// browsable tab), so it cannot be found by walking the view controllers hierarchy. It keeps it in
// its downloadVC property: ask for it through KVC to avoid importing myTabBarController.h here.
//
-(DownloadViewController*) findDownloadVCPropertyInViewController:(UIViewController*)vc {
    if (vc==nil) return nil;

    if ([vc respondsToSelector:NSSelectorFromString(@"downloadVC")]) {
        id res=[vc valueForKey:@"downloadVC"];
        if ([res isKindOfClass:[DownloadViewController class]]) return res;
    }

    if ([vc isKindOfClass:[UITabBarController class]]) {
        for (UIViewController *child in ((UITabBarController*)vc).viewControllers) {
            DownloadViewController *res=[self findDownloadVCPropertyInViewController:child];
            if (res) return res;
        }
        return nil;
    }
    if ([vc isKindOfClass:[UINavigationController class]]) {
        for (UIViewController *child in ((UINavigationController*)vc).viewControllers) {
            DownloadViewController *res=[self findDownloadVCPropertyInViewController:child];
            if (res) return res;
        }
        return nil;
    }
    for (UIViewController *child in vc.childViewControllers) {
        DownloadViewController *res=[self findDownloadVCPropertyInViewController:child];
        if (res) return res;
    }
    return nil;
}

-(void) loadDownloadController {
    if (downloadViewController) return;

    //1/ walk up from ourselves: we are in the same tab bar hierarchy
    for (UIViewController *vc=self; vc!=nil; vc=vc.parentViewController) {
        if ([vc respondsToSelector:NSSelectorFromString(@"downloadVC")]) {
            id res=[vc valueForKey:@"downloadVC"];
            if ([res isKindOfClass:[DownloadViewController class]]) {
                downloadViewController=res;
                return;
            }
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
        downloadViewController=[self findDownloadVCPropertyInViewController:window.rootViewController];
        if (downloadViewController) return;
        //last resort: the controller may still be part of the hierarchy
        downloadViewController=[self findChildOfClass:[DownloadViewController class] inViewController:window.rootViewController];
        if (downloadViewController) return;
    }
}

#pragma mark - Download requests build & queue

//
// HEAD request used to pick the right remote folder when a collection spreads its files over
// several bins. Blocking: MUST NOT be called from the main thread.
//
-(bool) remoteFileExistsAtURL:(NSString*)url {
    NSURL *nsurl=[NSURL URLWithString:url];
    if (nsurl==nil) return false;

    NSMutableURLRequest *request=[NSMutableURLRequest requestWithURL:nsurl
                                                        cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                                    timeoutInterval:15];
    [request setHTTPMethod:@"HEAD"];

    __block bool found=false;
    dispatch_semaphore_t sem=dispatch_semaphore_create(0);
    NSURLSessionDataTask *task=[[NSURLSession sharedSession] dataTaskWithRequest:request
                                                              completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        if ((error==nil)&&[response isKindOfClass:[NSHTTPURLResponse class]]) {
            NSInteger code=((NSHTTPURLResponse*)response).statusCode;
            if ((code>=200)&&(code<300)) found=true;
        }
        dispatch_semaphore_signal(sem);
    }];
    [task resume];
    dispatch_semaphore_wait(sem,dispatch_time(DISPATCH_TIME_NOW,(int64_t)20*NSEC_PER_SEC));

    return found;
}

//
// Synchronous form POST. Blocking: MUST NOT be called from the main thread.
//
-(NSData*) postFormToURL:(NSString*)url fields:(NSDictionary*)fields {
    NSURL *nsurl=[NSURL URLWithString:url];
    if (nsurl==nil) return nil;

    //x-www-form-urlencoded: only the unreserved characters are kept as-is
    NSCharacterSet *allowed=[NSCharacterSet characterSetWithCharactersInString:
                             @"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~"];
    NSMutableArray *pairs=[NSMutableArray array];
    for (NSString *key in fields) {
        NSString *encKey=[key stringByAddingPercentEncodingWithAllowedCharacters:allowed];
        NSString *encValue=[[fields objectForKey:key] stringByAddingPercentEncodingWithAllowedCharacters:allowed];
        if ((encKey==nil)||(encValue==nil)) return nil;
        [pairs addObject:[NSString stringWithFormat:@"%@=%@",encKey,encValue]];
    }

    NSMutableURLRequest *request=[NSMutableURLRequest requestWithURL:nsurl
                                                        cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                                    timeoutInterval:30];
    [request setHTTPMethod:@"POST"];
    [request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [request setHTTPBody:[[pairs componentsJoinedByString:@"&"] dataUsingEncoding:NSUTF8StringEncoding]];

    __block NSData *result=nil;
    dispatch_semaphore_t sem=dispatch_semaphore_create(0);
    NSURLSessionDataTask *task=[[NSURLSession sharedSession] dataTaskWithRequest:request
                                                              completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        if ((error==nil)&&data) result=data;
        dispatch_semaphore_signal(sem);
    }];
    [task resume];
    dispatch_semaphore_wait(sem,dispatch_time(DISPATCH_TIME_NOW,(int64_t)35*NSEC_PER_SEC));

    return result;
}

//the AMP browser normalizes the non breaking spaces when building the local path, do the same
//on the parsed cells before comparing them (cf RootViewControllerAMPWebParser.mm)
-(NSString*) ampCleanString:(NSString*)str {
    if (str==nil) return @"";
    return [[str stringByReplacingOccurrencesOfString:@"\u00a0" withString:@" "]
            stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
}

//
// The AMP artist folder on the server does not always match the composer handle used to build the
// local path, so the download link cannot be rebuilt directly: search the module by title and keep
// the row whose composer matches. Blocking: MUST NOT be called from the main thread.
//
-(NSString*) ampModuleURLForComposer:(NSString*)composer format:(NSString*)format title:(NSString*)title {
    NSData *urlData=[self postFormToURL:MDZ_AMP_SEARCH_URL fields:@{@"request":@"module",@"search":title}];
    if (urlData==nil) return nil;

    TFHpple *doc=[[TFHpple alloc] initWithHTMLData:urlData];
    NSArray *arr_files=[doc searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[1]"];
    NSArray *arr_composers=[doc searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[2]"];
    NSArray *arr_formats=[doc searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[3]"];

    NSInteger nb=[arr_formats count];
    if (nb==0) return nil;
    //the first rows can be unrelated ones, keep the last entries as the browser does
    if (([arr_files count]<nb)||([arr_composers count]<nb)) return nil;
    NSArray *files=[arr_files subarrayWithRange:NSMakeRange([arr_files count]-nb,nb)];
    NSArray *composers=[arr_composers subarrayWithRange:NSMakeRange([arr_composers count]-nb,nb)];

    NSString *wantedComposer=[self ampCleanString:composer];
    NSString *wantedTitle=[self ampCleanString:title];
    NSString *wantedFormat=[self ampCleanString:format];

    for (NSInteger i=0;i<nb;i++) {
        TFHppleElement *elFile=[files objectAtIndex:i];
        TFHppleElement *elComposer=[composers objectAtIndex:i];
        TFHppleElement *elFormat=[arr_formats objectAtIndex:i];

        if ([[self ampCleanString:elComposer.content] caseInsensitiveCompare:wantedComposer]!=NSOrderedSame) continue;
        if ([[self ampCleanString:elFile.content] caseInsensitiveCompare:wantedTitle]!=NSOrderedSame) continue;
        if ([wantedFormat length]&&([[self ampCleanString:elFormat.content] caseInsensitiveCompare:wantedFormat]!=NSOrderedSame)) continue;

        for (TFHppleElement *child in elFile.children) {
            NSString *href=[child objectForKey:@"href"];
            if (href) return [NSString stringWithFormat:@"%@/%@",MDZ_AMP_HOST_URL,href];
        }
    }
    return nil;
}

//the SNESmusic browser names the local file after the set long name (cf snes_spcSets.m),
//while the download URL expects the set short name
-(NSString*) snesmusicShortNameForSetName:(NSString*)setName {
    const char *target=[setName UTF8String];
    if (target==NULL) return nil;

    for (int i=0;i<snes_spc_entries;i++) {
        if (SNESmusic_names[i].longname&&(strcmp(SNESmusic_names[i].longname,target)==0))
            return [NSString stringWithUTF8String:SNESmusic_names[i].shortname];
    }
    //the local name may have been altered (case, trailing spaces, ...), retry loosely
    for (int i=0;i<snes_spc_entries;i++) {
        if (SNESmusic_names[i].longname==NULL) continue;
        NSString *longName=[NSString stringWithUTF8String:SNESmusic_names[i].longname];
        if ([longName caseInsensitiveCompare:setName]==NSOrderedSame)
            return [NSString stringWithUTF8String:SNESmusic_names[i].shortname];
    }
    return nil;
}

-(void) checkCreateForLocalPath:(NSString*)localPath {
    NSString *completePath=[[ModizFileHelper getFullPathForFilePath:localPath] stringByDeletingLastPathComponent];
    NSFileManager *fileMngr=[[NSFileManager alloc] init];
    [fileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:nil];
}

//build a download request, as used by the collections browsers (cf RootViewController<collection>.mm)
-(NSDictionary*) buildURLRequest:(NSString*)url localPath:(NSString*)localPath fileName:(NSString*)fileName isMODLAND:(int)isMODLAND {
    return @{@"type":@"url",@"url":url,@"local":localPath,@"name":fileName,@"ismodland":@(isMODLAND)};
}

-(NSDictionary*) buildFTPRequest:(NSString*)remotePath host:(NSString*)host localPath:(NSString*)localPath fileName:(NSString*)fileName isMODLAND:(int)isMODLAND {
    return @{@"type":@"ftp",@"remote":remotePath,@"host":host,@"local":localPath,@"name":fileName,@"ismodland":@(isMODLAND)};
}

//
// Rebuild the remote location of a missing entry.
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

    if ([baseDir isEqualToString:MDZ_AMP_BASEDIR]) {
        //local layout is Documents/AMP/<composer>/<FORMAT>.<title>.gz
        NSArray *comp=[localPath componentsSeparatedByString:@"/"];
        if ([comp count]<4) return nil;
        NSString *composer=[comp objectAtIndex:2];
        if ([composer length]==0) return nil;

        //the module name can itself contain '/' if the title does, keep everything after the composer
        NSString *modName=[[comp subarrayWithRange:NSMakeRange(3,[comp count]-3)] componentsJoinedByString:@"/"];
        modName=[modName stringByDeletingPathExtension];  //drop the .gz
        NSRange sep=[modName rangeOfString:@"."];
        if (sep.location==NSNotFound) return nil;
        NSString *format=[modName substringToIndex:sep.location];
        NSString *title=[modName substringFromIndex:sep.location+1];
        if ([title length]==0) return nil;

        NSString *url=[self ampModuleURLForComposer:composer format:format title:title];
        if (url==nil) return nil;  //not found in the search results

        return @[[self buildURLRequest:url localPath:localPath fileName:fileName isMODLAND:1]];
    }

    if ([baseDir isEqualToString:SNESmusic_BASEDIR]) {
        //local layout is Documents/SNESM/<set long name>.rsn
        //remote one is https://snesmusic.org/v2/download.php?spcNow=<set short name>
        NSString *setName=[[localPath lastPathComponent] stringByDeletingPathExtension];
        if ([setName length]==0) return nil;

        NSString *shortName=[self snesmusicShortNameForSetName:setName];
        if (shortName==nil) return nil;  //not in the sets list anymore
        NSString *escapedShortName=[shortName stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]];
        if (escapedShortName==nil) return nil;

        NSMutableArray *requests=[NSMutableArray array];

        //screenshot, stored next to the set as <set long name>.png, optional
        NSString *shotLocal=[[localPath stringByDeletingPathExtension] stringByAppendingString:@".png"];
        NSString *shotName=[[fileName stringByDeletingPathExtension] stringByAppendingString:@".png"];
        NSString *shotURL=[NSString stringWithFormat:@"%@%@.png",MDZ_SNESM_SCREENSHOT_URL,escapedShortName];
        [requests addObject:[self buildURLRequest:shotURL localPath:shotLocal fileName:shotName isMODLAND:3]];

        [requests addObject:[self buildURLRequest:[NSString stringWithFormat:@"%@%@",MDZ_SNESM_DOWNLOAD_URL,escapedShortName] localPath:localPath fileName:fileName isMODLAND:1]];
        return requests;
    }

    if ([baseDir isEqualToString:JOSHW_BASEDIR]) {
        //local layout is Documents/<webSite_baseDir>/<file>, webSite_baseDir being "JoshW/<system>"
        NSArray *comp=[localPath componentsSeparatedByString:@"/"];
        if ([comp count]<4) return nil;

        NSString *siteDir=[NSString stringWithFormat:@"%@/%@",[comp objectAtIndex:1],[comp objectAtIndex:2]];
        t_joshw_entry *site=NULL;
        for (int i=0;i<joshw_subsites_size;i++) {
            if ([joshw_subsites[i].webSite_baseDir isEqualToString:siteDir]) {
                site=&joshw_subsites[i];
                break;
            }
        }
        if (site==NULL) return nil;  //unknown system

        //the local name is the percent decoded remote one, and can itself contain '/'
        NSString *modName=[[comp subarrayWithRange:NSMakeRange(3,[comp count]-3)] componentsJoinedByString:@"/"];
        if ([modName length]==0) return nil;
        NSString *encName=[modName stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLPathAllowedCharacterSet]];
        if (encName==nil) return nil;

        //some systems keep everything at the root
        if (!site->has_letter_index) {
            return @[[self buildURLRequest:[NSString stringWithFormat:@"%@/%@",site->webSite_URL,encName] localPath:localPath fileName:fileName isMODLAND:1]];
        }

        //otherwise files are binned by initial: 0-9, a, b, ... z
        unichar initial=[[modName lowercaseString] characterAtIndex:0];
        NSString *letter;
        if ((initial>='a')&&(initial<='z')) letter=[NSString stringWithFormat:@"%C",initial];
        else letter=@"0-9";

        NSMutableArray *candidates=[NSMutableArray array];
        [candidates addObject:[NSString stringWithFormat:@"%@/%@/%@",site->webSite_URL,letter,encName]];
        //a few systems have extra bins (zzz_prototypes, zzz_unlicensed, ...) not covered by the initial
        for (NSString *extra in site->extra_index) {
            [candidates addObject:[NSString stringWithFormat:@"%@/%@/%@",site->webSite_URL,extra,encName]];
        }

        if ([candidates count]==1)
            return @[[self buildURLRequest:[candidates firstObject] localPath:localPath fileName:fileName isMODLAND:1]];

        //several bins possible: ask the server which one holds the file
        for (NSString *candidate in candidates) {
            if ([self remoteFileExistsAtURL:candidate])
                return @[[self buildURLRequest:candidate localPath:localPath fileName:fileName isMODLAND:1]];
        }
        return nil;  //not found in any bin
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

#pragma mark - Missing entries scan

//
// Walk a list of playlist fullpaths, keep the missing ones and build their download requests.
// stats receives the counters used to build the summary message.
//
-(NSArray*) buildDownloadRequestsForPaths:(NSArray*)fullpaths stats:(NSMutableDictionary*)stats {
    NSMutableArray *requests=[NSMutableArray array];
    NSMutableSet *alreadyChecked=[NSMutableSet set];
    NSMutableSet *unresolvedCollections=[NSMutableSet set];
    int nb_paths=(int)[fullpaths count];
    int nb_queued=0,nb_present=0,nb_notonline=0,nb_unresolved=0;

    for (int i=0;i<nb_paths;i++) {
        if ((i%16)==0) {
            dispatch_async(dispatch_get_main_queue(), ^(void){
                [self updateWaitingDetail:[NSString stringWithFormat:@"%d/%d",i,nb_paths]];
            });
        }

        //remove the archive index (@) and subsong index (?) suffixes if any
        NSString *localPath=[ModizFileHelper getFullCleanFilePath:[fullpaths objectAtIndex:i]];
        if (localPath==nil) continue;
        if ([alreadyChecked containsObject:localPath]) continue;
        [alreadyChecked addObject:localPath];

        NSString *baseDir=[self onlineCollectionBaseDirForLocalPath:localPath];
        if (baseDir==nil) {
            nb_notonline++;
            continue;
        }

        if ([self isLocalFileAvailable:localPath]) {
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

    [stats setObject:@(nb_queued) forKey:@"queued"];
    [stats setObject:@(nb_present) forKey:@"present"];
    [stats setObject:@(nb_notonline) forKey:@"notonline"];
    [stats setObject:@(nb_unresolved) forKey:@"unresolved"];
    [stats setObject:unresolvedCollections forKey:@"collections"];

    return requests;
}

-(NSString*) downloadScanSummaryFromStats:(NSDictionary*)stats {
    NSMutableArray *msgLines=[NSMutableArray array];
    [msgLines addObject:[NSString stringWithFormat:NSLocalizedString(@"%d file(s) queued for download.",@""),[[stats objectForKey:@"queued"] intValue]]];
    [msgLines addObject:[NSString stringWithFormat:NSLocalizedString(@"%d file(s) already available.",@""),[[stats objectForKey:@"present"] intValue]]];
    [msgLines addObject:[NSString stringWithFormat:NSLocalizedString(@"%d file(s) not from an online collection.",@""),[[stats objectForKey:@"notonline"] intValue]]];

    int nb_unresolved=[[stats objectForKey:@"unresolved"] intValue];
    if (nb_unresolved) {
        NSArray *sortedCollections=[[[stats objectForKey:@"collections"] allObjects] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
        [msgLines addObject:[NSString stringWithFormat:NSLocalizedString(@"%d file(s) cannot be resolved (%@), browse the collection to download them.",@""),
                             nb_unresolved,[sortedCollections componentsJoinedByString:@", "]]];
    }
    return [msgLines componentsJoinedByString:@"\n"];
}

-(void) scanPathsAndDownloadMissingEntries:(NSArray*)fullpaths {
    [self hideWaitingCancel];
    [self hideWaitingProgress];
    [self updateWaitingTitle:NSLocalizedString(@"Checking playlists",@"")];
    [self updateWaitingDetail:@""];
    [self showWaiting];

    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
        //Background Thread
        NSMutableDictionary *stats=[NSMutableDictionary dictionary];
        NSArray *requests=[self buildDownloadRequestsForPaths:fullpaths stats:stats];

        dispatch_async(dispatch_get_main_queue(), ^(void){
            //Run UI Updates
            [self queueDownloadRequests:requests];
            [self hideWaiting];
            if ([self respondsToSelector:@selector(refreshViewAfterDownload)]) [self performSelector:@selector(refreshViewAfterDownload)];
            [self showAlertMsg:NSLocalizedString(@"Info",@"") message:[self downloadScanSummaryFromStats:stats]];
        });
    });
}

-(void) confirmDownloadMissingEntriesForPaths:(NSArray*)fullpaths {
    [self loadDownloadController];
    if (downloadViewController==nil) {
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Download manager is not available.",@"")];
        return;
    }

    NSString *message=[NSString stringWithFormat:@"%@\n\n%@\n%@",
                       NSLocalizedString(@"Queue the download of the missing files coming from online collections ?",@""),
                       NSLocalizedString(@"Supported collections:",@""),
                       MDZ_RESOLVABLE_COLLECTIONS];

    UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Info",@"")
                                                                  message:message
                                                           preferredStyle:UIAlertControllerStyleAlert];

    UIAlertAction* checkAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Check",@"") style:UIAlertActionStyleDefault
                                                       handler:^(UIAlertAction * action) {
        [self scanPathsAndDownloadMissingEntries:fullpaths];
    }];
    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
                                                        handler:^(UIAlertAction * action) {
    }];

    [alert addAction:cancelAction];
    [alert addAction:checkAction];
    [self presentViewController:alert animated:YES completion:nil];
}

#endif /* OnlineCollectionsCommonFunctions_h */
