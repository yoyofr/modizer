//
//  CoverScrapper.m
//  modizer
//
//  Created by Yohann Magnien David on 29/01/2026.
//

#import "CoverScrapper.h"
#import "TFHpple.h"
#import "ModizerConstants.h"
#import "ModizFileHelper.h"

@implementation CoverScrapper

#pragma mark - Public Methods

- (instancetype)init {
    self = [super init];
    if (self) {
        _matcher=[[StringMatcher alloc] init];
    }
    return self;
}


- (void)getImgfromImgGrabber:(NSString*)grabber_url search_label:(NSString*)search_label label:(NSString*)label fullpath:(NSString*)fullpath completion:(void (^)(void))block {
    static int no_reentrant=0;
    if (no_reentrant) return;
    no_reentrant=1;
    if ([grabber_url containsString:@"thegamesdb"]) [self getImgfromTGDB:grabber_url search_label:search_label label:label fullpath:fullpath completion:block];
    
    if ([grabber_url containsString:@"mobygames"]) [self getImgfromMobyG:grabber_url search_label:search_label label:label fullpath:fullpath completion:block];
    no_reentrant=0;
}

#pragma mark - Private Methods - String Processing

- (NSString *)removeParenthesesAndBrackets:(NSString *)input {
    if (!input) return nil;
    
    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:@"[\\(\\[].*?[\\)\\]]"
                                                                           options:0
                                                                             error:nil];
    NSString *result = [regex stringByReplacingMatchesInString:input
                                                        options:0
                                                          range:NSMakeRange(0, input.length)
                                                   withTemplate:@""];
    
    // Nettoyer tous les espaces multiples (pas seulement les doubles)
    NSRegularExpression *spaceRegex = [NSRegularExpression regularExpressionWithPattern:@"\\s+"
                                                                                 options:0
                                                                                   error:nil];
    result = [spaceRegex stringByReplacingMatchesInString:result
                                                  options:0
                                                    range:NSMakeRange(0, result.length)
                                             withTemplate:@" "];
    
    // Trim les espaces en début et fin
    return [result stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
}

- (NSString *)removeParentheses:(NSString *)input {
    if (!input) return nil;
    
    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:@"[\\(].*?[\\)]"
                                                                           options:0
                                                                             error:nil];
    NSString *result = [regex stringByReplacingMatchesInString:input
                                                        options:0
                                                          range:NSMakeRange(0, input.length)
                                                   withTemplate:@""];
    
    // Nettoyer tous les espaces multiples (pas seulement les doubles)
    NSRegularExpression *spaceRegex = [NSRegularExpression regularExpressionWithPattern:@"\\s+"
                                                                                 options:0
                                                                                   error:nil];
    result = [spaceRegex stringByReplacingMatchesInString:result
                                                  options:0
                                                    range:NSMakeRange(0, result.length)
                                             withTemplate:@" "];
    
    // Trim les espaces en début et fin
    return [result stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
}


- (int)getImgfromTGDB:(NSString*)grabber_url search_label:(NSString*)search_label label:(NSString*)label fullpath:(NSString*)fullpath completion:(void (^)(void))block {
    //check if cover exists
    NSString *cleanName=[self removeParenthesesAndBrackets:[search_label stringByDeletingPathExtension]];
    if ([cleanName containsString:@","]) {
        cleanName=[cleanName substringToIndex:[cleanName rangeOfString:@","].location];
    }
    NSString *url_img=[[NSString stringWithFormat:grabber_url,[cleanName stringByReplacingOccurrencesOfString:@" " withString:@"+"]] stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]];
    //MDZILog("url gamedb: %@",url_img);
    
    NSURL *url=[NSURL URLWithString:url_img];
    NSData *reqData = [NSData dataWithContentsOfURL:url];
    
    TFHpple *doc = [[TFHpple alloc] initWithHTMLData:reqData];
    NSMutableArray *arr_img=[NSMutableArray arrayWithArray:[doc searchWithXPathQuery:@"/html/body//img[@class='card-img-top']/@src"]];
    NSMutableArray *arr_title=[NSMutableArray arrayWithArray:[doc searchWithXPathQuery:@"/html/body//img[@class='card-img-top']/@alt"]];
    if ((arr_img!=nil) && [arr_img count]) {
        TFHppleElement *el;
        
        //Init game names list
        NSMutableArray *gameNames=[NSMutableArray arrayWithCapacity:[arr_title count]];
        NSString *lbl;
        for (int i=0;i<[arr_title count];i++) {
            el=[arr_title objectAtIndex:i];
            lbl=[[el content] stringByReplacingOccurrencesOfString:@" cover" withString:@""];
            [gameNames insertObject:lbl atIndex:i];
        }
        NSInteger index = [self.matcher bestMatchIndexFor:cleanName inArray:gameNames minimumScore:0.5];
        
        if (index != NSNotFound) {
            //NSLog(@"Best match: %@", gameNames[index]);
            el=[arr_img objectAtIndex:index];
            url_img=[el content];
            NSString *imgPath;
            if ([url_img pathExtension] && [[url_img pathExtension] length]) {
                imgPath=[[fullpath stringByDeletingPathExtension] stringByAppendingFormat:@".%@",[[url_img pathExtension] lowercaseString]];
            } else {
                imgPath=[[fullpath stringByDeletingPathExtension] stringByAppendingFormat:@".png"];
            }
            

            url = [NSURL URLWithString:url_img];
            NSURLSession *session = [NSURLSession sharedSession];
            NSURLSessionDataTask *task =
            [session dataTaskWithURL:url
                   completionHandler:^(NSData * _Nullable data,
                                       NSURLResponse * _Nullable response,
                                       NSError * _Nullable error)
            {
                if (error) {
                    MDZELog("Erreur réseau : %@", error);
                    return;
                }

                if (!data) {
                    MDZELog("Aucune donnée reçue");
                    return;
                }
                NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
                if (httpResponse.statusCode == 404) {
                    MDZELog("Fichier non trouvé (404)");
                    // Traiter le cas 404
                    return;
                }
                
                if (httpResponse.statusCode >= 400) {
                    MDZELog("Erreur HTTP: %ld", (long)httpResponse.statusCode);
                    // Traiter les autres erreurs HTTP
                    return;
                }
            
                if ([imgPath containsString:[ModizFileHelper getAppHomeDirectory]]) {
                    [data writeToFile:[NSString stringWithFormat:@"%@",imgPath] atomically:NO];
                } else {
                    [data writeToFile:[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getAppHomeDirectory],imgPath] atomically:NO];
                }
                    
                    dispatch_async(dispatch_get_main_queue(), block);
                }];
            
            [task resume];
            
            return 1;
        }
        
//                [downloadViewController addURLToDownloadList:url_img fileName:imgName filePath:imgPath filesize:-1 isMODLAND:0 usePrimaryAction:mClickedPrimAction];
    }
    cleanName=[self removeParentheses:[search_label stringByDeletingPathExtension]];
        int found=0;
        if ([cleanName containsString:@"-"]) {
            NSString *search_labelA=[search_label substringToIndex:[cleanName rangeOfString:@"-"].location];
            NSString *search_labelB=[search_label substringFromIndex:[cleanName rangeOfString:@"-"].location+1];
            found=[self getImgfromTGDB:grabber_url search_label:search_labelA label:label fullpath:fullpath completion:block];
            if (!found) found=[self getImgfromTGDB:grabber_url search_label:search_labelB label:label fullpath:fullpath completion:block];
            if (!found) {
                if ([cleanName containsString:@"["]&&[cleanName containsString:@"]"]) {
                    NSString *search_labelA=[cleanName substringFromIndex:[cleanName rangeOfString:@"["].location+1];
                    search_labelA=[search_labelA substringToIndex:[search_labelA rangeOfString:@"]"].location];
                    found=[self getImgfromTGDB:grabber_url search_label:search_labelA label:label fullpath:fullpath completion:block];
                }
            }
        } else {
            
            if ([cleanName containsString:@"["]&&[cleanName containsString:@"]"]) {
                NSString *search_labelA=[cleanName substringFromIndex:[cleanName rangeOfString:@"["].location+1];
                search_labelA=[search_labelA substringToIndex:[search_labelA rangeOfString:@"]"].location];
                found=[self getImgfromTGDB:grabber_url search_label:search_labelA label:label fullpath:fullpath completion:block];
            }
            if (!found) {
                //try by removing last part of search label if big enough
                NSMutableArray *arr=[NSMutableArray arrayWithArray:[cleanName componentsSeparatedByString:@" "]];
                if ([arr count]>2) {
                    [arr removeLastObject];
                    cleanName=[arr componentsJoinedByString:@" "];
                    [self getImgfromTGDB:grabber_url search_label:cleanName label:label fullpath:fullpath completion:block];
                }
            }
        }
    return 0;
}

- (NSString *)extractInternalURLFromMobyGames:(NSString *)pageURL {
    NSURL *url = [NSURL URLWithString:pageURL];
    NSData *reqData = [NSData dataWithContentsOfURL:url];
    
    if (!reqData) return nil;
    
    NSString *html = [[NSString alloc] initWithData:reqData encoding:NSUTF8StringEncoding];
    
    // Regex pour capturer le JSON dans :initial-values='...'
    NSRegularExpression *regex = [NSRegularExpression
        regularExpressionWithPattern:@":initial-values='([^']+)'"
        options:0
        error:nil];
    
    NSTextCheckingResult *match = [regex firstMatchInString:html
                                                    options:0
                                                      range:NSMakeRange(0, [html length])];
    
    if (!match || match.numberOfRanges < 2) {
        NSLog(@"⚠️ Pas de :initial-values trouvé");
        return nil;
    }
    
    // Extraire et nettoyer le JSON
    NSString *jsonString = [html substringWithRange:[match rangeAtIndex:1]];
    jsonString = [jsonString stringByReplacingOccurrencesOfString:@"&quot;" withString:@"\""];
    jsonString = [jsonString stringByReplacingOccurrencesOfString:@"&#x27;" withString:@"'"];
    jsonString = [jsonString stringByReplacingOccurrencesOfString:@"&amp;" withString:@"&"];
    
    // Parser le JSON
    NSData *jsonData = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
    NSError *jsonError;
    NSDictionary *json = [NSJSONSerialization JSONObjectWithData:jsonData
                                                         options:0
                                                           error:&jsonError];
    
    if (jsonError) {
        NSLog(@"⚠️ Erreur parsing JSON: %@", jsonError.localizedDescription);
        return nil;
    }
    
    // Extraire l'URL
    NSArray *games = json[@"games"];
    if (games && [games count] > 0) {
        return games[0][@"internal_url"];
    }
    
    return nil;
}

- (NSString *)extractCoverImageFromMobyGames:(NSString *)pageURL {
    NSURL *url = [NSURL URLWithString:pageURL];
    NSData *reqData = [NSData dataWithContentsOfURL:url];
    
    if (!reqData) return nil;
    
    NSString *html = [[NSString alloc] initWithData:reqData encoding:NSUTF8StringEncoding];
    
    // Méthode 1 : Extraire depuis les meta tags OpenGraph
    NSRegularExpression *regex = [NSRegularExpression
        regularExpressionWithPattern:@"<meta property=\"og:image\" content=\"([^\"]+)\""
        options:0
        error:nil];
    
    NSTextCheckingResult *match = [regex firstMatchInString:html
                                                    options:0
                                                      range:NSMakeRange(0, [html length])];
    
    if (match && match.numberOfRanges > 1) {
        NSString *imageURL = [html substringWithRange:[match rangeAtIndex:1]];
        NSLog(@"✅ Image trouvée (meta): %@", imageURL);
        return imageURL;
    }
    
    // Méthode 2 : Si pas de meta, chercher dans le JSON du Web Component player-tools
    regex = [NSRegularExpression
        regularExpressionWithPattern:@"cover-url=\"([^\"]+)\""
        options:0
        error:nil];
    
    match = [regex firstMatchInString:html options:0 range:NSMakeRange(0, [html length])];
    
    if (match && match.numberOfRanges > 1) {
        NSString *imageURL = [html substringWithRange:[match rangeAtIndex:1]];
        NSLog(@"✅ Image trouvée (component): %@", imageURL);
        return imageURL;
    }
    
    return nil;
}

- (int)getImgfromMobyG:(NSString*)grabber_url search_label:(NSString*)search_label label:(NSString*)label fullpath:(NSString*)fullpath completion:(void (^)(void))block {
    //check if cover exists
    NSString *cleanName=[self removeParenthesesAndBrackets:[search_label stringByDeletingPathExtension]];
    if ([cleanName containsString:@","]) {
        cleanName=[cleanName substringToIndex:[cleanName rangeOfString:@","].location];
    }
    NSString *url_img=[[NSString stringWithFormat:grabber_url,cleanName] stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]];
    
    // Utilisation
    url_img = [self extractInternalURLFromMobyGames:url_img];
    if (url_img) {
//        MDZILog("✅ Game URL: %@", url_img);
        
        url_img = [self extractCoverImageFromMobyGames:url_img];

        if (url_img) {
            
            NSString *imgPath=[[fullpath stringByDeletingPathExtension] stringByAppendingFormat:@".%@",[[url_img pathExtension] lowercaseString]];
            
            
            
            NSURL *url = [NSURL URLWithString:url_img];
            NSURLSession *session = [NSURLSession sharedSession];
            NSURLSessionDataTask *task =
            [session dataTaskWithURL:url
                   completionHandler:^(NSData * _Nullable data,
                                       NSURLResponse * _Nullable response,
                                       NSError * _Nullable error) {
                if (error) {
                    MDZELog("Erreur réseau : %@", error);
                    return;
                }
                
                if (!data) {
                    MDZELog("Aucune donnée reçue");
                    return;
                }
                NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
                if (httpResponse.statusCode == 404) {
                    MDZELog("Fichier non trouvé (404)");
                    // Traiter le cas 404
                    return;
                }
                
                if (httpResponse.statusCode >= 400) {
                    MDZELog("Erreur HTTP: %ld", (long)httpResponse.statusCode);
                    // Traiter les autres erreurs HTTP
                    return;
                }
                if ([imgPath containsString:[ModizFileHelper getAppHomeDirectory]]) {
                    [data writeToFile:[NSString stringWithFormat:@"%@",imgPath] atomically:NO];
                } else {
                    [data writeToFile:[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getAppHomeDirectory],imgPath] atomically:NO];
                }
                
                dispatch_async(dispatch_get_main_queue(), block);
            }];
            
            [task resume];
            
            return 1;
            
        }
    } else {
        if ([cleanName containsString:@"-"]) {
            cleanName=[cleanName substringToIndex:[cleanName rangeOfString:@"-"].location];
            [self getImgfromMobyG:grabber_url search_label:cleanName label:label fullpath:fullpath completion:block];
        } else if ([cleanName containsString:@"OPN"]) {
            cleanName=[cleanName stringByReplacingOccurrencesOfString:@"OPNA" withString:@""];
            cleanName=[cleanName stringByReplacingOccurrencesOfString:@"OPN" withString:@""];
            [self getImgfromMobyG:grabber_url search_label:cleanName label:label fullpath:fullpath completion:block];
        } else if ([cleanName containsString:@"AD&D"]) {
            cleanName=[cleanName stringByReplacingOccurrencesOfString:@"AD&D" withString:@""];
            [self getImgfromMobyG:grabber_url search_label:cleanName label:label fullpath:fullpath completion:block];
        }
    }
    return 0;
}

@end
