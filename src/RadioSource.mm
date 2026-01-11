//
//  RadioSource.mm
//  modizer
//
//  Created by Yohann Magnien David on 10/01/2026.
//
#define RS_AMP_MAX_COMPOSER_ID 19822
#define RS_DOWNLOAD_MAX_RETRY_COUNT 16

#import "ModizerConstants.h"
#import "ModizFileHelper.h"
#import "RadioSource.h"
#import "TFHpple.h"

#include <stdlib.h>

#define RS_QUEUE_SIZE 5

NS_ASSUME_NONNULL_BEGIN

@implementation RadioSource

@synthesize mRadioSource,mPendingNewFileToPlay,mRadioSource_mode,mRetryCount;
@synthesize detailVC;
@synthesize mActive,mFilesList,mFilesExistInLibrary,mSourceData;

- (instancetype)init {
    self = [super init];
    if (self) {
        mRadioSource=RS_NONE;
        mRadioSource_mode=0;
        mPendingNewFileToPlay=0;
        mActive=NO;
        mRetryCount=0;
        mFilesList = [NSMutableArray arrayWithCapacity:5];
        mFilesExistInLibrary = [NSMutableArray arrayWithCapacity:5];
        mSourceData = [NSMutableArray array];
        [self cleanFiles];
    }
    return self;
}

- (void)downloadFileFromURL:(NSString *)urlString rSource:(t_radioSource)rSource slot:(int)slot composer:(NSString*)composer {
    NSURL *url = [NSURL URLWithString:urlString];
    
    NSURLSessionConfiguration *config = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSURLSession *session = [NSURLSession sessionWithConfiguration:config
                                                          delegate:self
                                                     delegateQueue:nil];
    
    NSURLSessionDownloadTask *downloadTask = [session downloadTaskWithURL:url
                                                        completionHandler:^(NSURL *location, NSURLResponse *response, NSError *error) {
        if (error) {
            MDZELog("Error downloading: %@", error.localizedDescription);
            return;
        }
        
        // Récupérer le nom de fichier suggéré par le serveur
        NSString *suggestedFilename = response.suggestedFilename;
        if (!suggestedFilename) {
            suggestedFilename = [url lastPathComponent];
        }
                
        NSString *collection=[self getSourceName:rSource];
        
        // Destination finale
        NSString *destinationPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%d/%@/%@/%@",slot,collection,composer,suggestedFilename];
        
        NSURL *destinationURL = [NSURL fileURLWithPath:destinationPath];
        
        // Déplacer le fichier temporaire vers la destination
        NSFileManager *fileManager = [NSFileManager defaultManager];
        
        [fileManager createDirectoryAtPath:[destinationPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&error];
        [ModizFileHelper addSkipBackupAttributeToItemAtPath:destinationPath];
        
        // Supprimer le fichier existant si nécessaire
        if ([fileManager fileExistsAtPath:destinationPath]) {
            [fileManager removeItemAtPath:destinationPath error:nil];
        }
        
        NSError *moveError = nil;
        [fileManager moveItemAtURL:location toURL:destinationURL error:&moveError];
        
        if (moveError) {
            MDZELog("Error moving: %@", moveError.localizedDescription);
        } else {
            //NSLog(@"Fichier téléchargé: %@", destinationPath);
            dispatch_async(dispatch_get_main_queue(), ^{
                // Mise à jour UI si nécessaire
                if ((mPendingNewFileToPlay==1)&&(slot==0)) [self startNextEntry];
            });
            
        }
    }];
    
    [downloadTask resume];
}

-(void)getNewAMPFile:(int)slot {
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    NSError *err;
    NSString *fileURL;
    
    //clean slot
    NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%d/",slot];
    [mFileMngr removeItemAtPath:localPath error:&err];
    //create tmp dir
    [mFileMngr createDirectoryAtPath:localPath withIntermediateDirectories:TRUE attributes:nil error:&err];
    [ModizFileHelper addSkipBackupAttributeToItemAtPath:localPath];
    
    //get URL
    if ((mRadioSource_mode==0)||(mRadioSource_mode==1)) {
        //Random composer and then file
        //

        int mAMP_max_compID=RS_AMP_MAX_COMPOSER_ID;
        int composer_id=arc4random_uniform(mAMP_max_compID)+1;
        
        if (mRadioSource_mode==1) {
            if ([mSourceData count]) composer_id=atoi([mSourceData[0] UTF8String]);
            else {
                MDZELog("Radio: source is AMP, mode is 1 and no composer ID is available. using random value");
            }
        }
        fileURL=[NSString stringWithFormat:@"https://amp.dascene.net/detail.php?detail=modules&view=%d",composer_id];
        
        NSURL *url = [NSURL URLWithString:[NSString stringWithFormat:@"%@",fileURL]];
        
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
            
            TFHpple *doc       = [[TFHpple alloc] initWithHTMLData:data];

            NSArray *arr_url_composerList=[doc searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[2]"];
            NSArray *arr_url_modulesNb=[doc searchWithXPathQuery:@"//td[@class='descript' and contains(normalize-space(.), 'Modules')]/following-sibling::td[1]/a"];
            NSArray *arr_url_fileList=[doc searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[1]/a"];
            
            arr_url_composerList=[arr_url_composerList subarrayWithRange:NSMakeRange([arr_url_composerList count]-[arr_url_fileList count], [arr_url_fileList count])];
            
            TFHppleElement *el;
            int mod_id=-1;
            //add entries for each group & country
            if ([arr_url_modulesNb count]>0) {
                
                el=[arr_url_composerList objectAtIndex:0];
                NSString *composer=[NSString stringWithString:el.content];
                MDZILog("composer: %@",composer);
                
                el=[arr_url_modulesNb objectAtIndex:0];
                int mod_nb=atoi([el.content UTF8String]);
                MDZILog("modules: %d",mod_nb);
                
                mod_id=arc4random_uniform(mod_nb);
                el=[arr_url_fileList objectAtIndex:mod_id];
                NSString *fileModURL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el objectForKey:@"href"]];
                
                if ([composer length] && [fileModURL length]) {
                    mRetryCount=0;
                    [self downloadFileFromURL:fileModURL rSource:RS_COLLECTION_AMP slot:slot composer:composer];
                } else {
                    //issue, try another one if max try isn't reached
                    mRetryCount++;
                    if (mRetryCount<RS_DOWNLOAD_MAX_RETRY_COUNT) [self getNewAMPFile:slot];
                    else {
                        //stop radio
                        dispatch_async(dispatch_get_main_queue(), ^{
                            // Mise à jour UI si nécessaire
                            [self stop];
                        });
                    }
                }
            } else {
                //no module
                //issue, try another one if max try isn't reached
                mRetryCount++;
                if (mRetryCount<RS_DOWNLOAD_MAX_RETRY_COUNT) [self getNewAMPFile:slot];
                else {
                    //stop radio
                    dispatch_async(dispatch_get_main_queue(), ^{
                        // Mise à jour UI si nécessaire
                        [self stop];
                    });
                }
            }
        }];
        
        [task resume];
    }
}

-(bool) isInLibrary:(int)slot {
    [self scanForPlayableFiles];
    if (slot<[mFilesExistInLibrary count]) return [(NSNumber*)mFilesExistInLibrary[slot] boolValue];
    return false;
}

-(void)scanForPlayableFiles {
    NSFileManager *mFileMngr = [[NSFileManager alloc] init];
    NSString *localPath = [[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/"];
    
    [mFilesList removeAllObjects];
    
    // Énumérateur récursif
    NSDirectoryEnumerator *enumerator = [mFileMngr enumeratorAtPath:localPath];
    
    for (NSString *relativePath in enumerator) {
        NSString *fullPath = [localPath stringByAppendingPathComponent:relativePath];
        
        // Récupérer les attributs pour vérifier si c'est un fichier ou un dossier
        NSDictionary *attributes = [enumerator fileAttributes];
        NSString *fileType = attributes[NSFileType];
        
        // Ne traiter que les fichiers (pas les dossiers)
        if ([fileType isEqualToString:NSFileTypeRegular]) {
            if ([ModizFileHelper isPlayableFile:fullPath]) {
                [mFilesList addObject:relativePath]; // Chemin relatif incluant les sous-dossiers
            }
        }
    }
    
    // Trier par chemin complet (ordre alphabétique insensible à la casse)
    [mFilesList sortUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
    
    [mFilesExistInLibrary removeAllObjects];
    
    for (NSString *relativePath in mFilesList) {
        NSString *libPath = [[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/Documents/%@",[relativePath substringFromIndex:[relativePath rangeOfString:@"/"].location+1]];
        
        if ([mFileMngr fileExistsAtPath:libPath isDirectory:nil]) {
            [mFilesExistInLibrary addObject:[NSNumber numberWithBool:true]];
        } else [mFilesExistInLibrary addObject:[NSNumber numberWithBool:false]];
    }
}

-(void) fetchNewFileFromSource:(int)slot {
    switch (mRadioSource) {
        case RS_COLLECTION_AMP:
            [self getNewAMPFile:slot];
            break;
        default:
            break;
    }
}

-(void) startNextEntry {
    mPendingNewFileToPlay=0;
    
    //update files list
    [self scanForPlayableFiles];
    
    if ([mFilesList count]) {
        NSMutableArray *array_label = [[NSMutableArray alloc] init];
        NSMutableArray *array_path = [[NSMutableArray alloc] init];
        
        [array_label addObject:mFilesList[0]];
        
        NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%@",mFilesList[0]];
        [array_path addObject:localPath];
        
        [detailVC play_listmodules:array_label start_index:0 path:array_path];
    }
}

-(void)fetchRenewFilesAndStart {
    //update files list
    [self scanForPlayableFiles];
    
    if ([mFilesList count]) {
        NSFileManager *mFileMngr=[[NSFileManager alloc] init];
        NSError *error;
        //remove 1st one
        NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/0"];
        [mFileMngr removeItemAtPath:localPath error:&error];
        if (error) {
            MDZELog("Error: %@", error.localizedDescription);
        }
        [mFilesList removeObjectAtIndex:0];
        [mFilesExistInLibrary removeObjectAtIndex:0];
        //rename others
        for (int i=0;i<[mFilesList count];i++) {
            NSString *fromPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%@",[[[mFilesList objectAtIndex:i] componentsSeparatedByString:@"/"] firstObject]];
            NSString *toPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%d",i];
            MDZILog("move from %@\nto %@",fromPath,toPath);
            [mFileMngr moveItemAtPath:fromPath toPath:toPath error:&error];
            if (error) {
                MDZELog("Error moving tmpRadio/%d to tmpRadio/%d : %@",i+1,i, error.localizedDescription);
            }
        }
        
        //update files list
        [self scanForPlayableFiles];
    }
    
    mPendingNewFileToPlay=1;
    //if there's at least 1 entry, start it
    if ([mFilesList count]) {
        [self startNextEntry];
    }
    
    int max_download=2;
    for (int i=(int)[mFilesList count];i<RS_QUEUE_SIZE;i++) {
        //initiate a new download
        [self fetchNewFileFromSource:i];
        max_download--;
        if (!max_download) break;
    }
}

-(NSString *) getSourceName:(t_radioSource)rSource {
    switch (rSource) {
        default:
        case RS_NONE:
            return nil;
        case RS_COLLECTION_AMP:
            return @"AMP";
        case RS_COLLECTION_ASMA:
            return @"ASMA";
        case RS_COLLECTION_HVSC:
            return @"HVSC";
        case RS_COLLECTION_CGSC:
            return @"CGSC";
        case RS_COLLECTION_MODLAND:
            return @"MODLAND";
    }
    return nil;
}


-(NSString *) radioSourceName {
    switch (mRadioSource) {
        default:
        case RS_NONE:
            return nil;
        case RS_COLLECTION_AMP:
            return @"AMP";
        case RS_COLLECTION_ASMA:
            return @"ASMA";
        case RS_COLLECTION_HVSC:
            return @"HVSC";
        case RS_COLLECTION_CGSC:
            return @"CGSC";
        case RS_COLLECTION_MODLAND:
            return @"MODLAND";
    }
    return nil;
}

-(void) cleanFiles {
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    NSError *error;

    NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio"];
    [mFileMngr removeItemAtPath:localPath error:&error];
    if (error) {
        MDZELog("Error cleanFiles: %@", error.localizedDescription);
    }
}

-(void) moveNext {
    [self fetchRenewFilesAndStart];
}

-(void) activate {
    mActive=YES;
    [self moveNext];
}
-(void) stop {
    mActive=NO;
    [self cleanFiles];
}
-(bool) isActive {
    return mActive;
}

-(int) queueSize {
    [self scanForPlayableFiles];
    return (int)[mFilesList count];
}

-(NSString *) getQueueLabel:(int)slot {
    NSString *result=nil;
    [self scanForPlayableFiles];
    if ([mFilesList count]>=slot) {
        result=[mFilesList objectAtIndex:slot];
        NSArray *arr=[result componentsSeparatedByString:@"/"];
        if (mRadioSource==RS_COLLECTION_AMP) {
            if ([arr count]>=4) result=[NSString stringWithFormat:@"%@\nby\n%@",[arr[3] stringByDeletingPathExtension],arr[2]];
            else result=[NSString stringWithFormat:@"%@",[[arr lastObject] stringByDeletingPathExtension]];
        }
    }
    return result;
}
-(bool) saveFileToLibrary:(int)slot {
    bool ret=false;
    [self scanForPlayableFiles];
    if ([mFilesList count]>=slot) {
        NSString *relativePath=[mFilesList objectAtIndex:slot];
        if (mRadioSource==RS_COLLECTION_AMP) {
            NSFileManager *fileManager = [[NSFileManager alloc] init];
            NSError *error=nil;
            NSString *fromPath = [[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%@",relativePath];
            
            NSString *toPath = [[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/Documents/%@",[relativePath substringFromIndex:[relativePath rangeOfString:@"/"].location+1]];
            //MDZILog("from %@\nto %@",fromPath,toPath);
            // Récupère le dossier parent
            NSString *destinationDir = [toPath stringByDeletingLastPathComponent];

            // Crée les dossiers intermédiaires si besoin
            if (![fileManager fileExistsAtPath:destinationDir]) {
                BOOL created = [fileManager createDirectoryAtPath:destinationDir
                                       withIntermediateDirectories:YES
                                                        attributes:nil
                                                             error:&error];
                if (!created) {
                    NSLog(@"Erreur création dossier: %@", error);
                    ret=false;
                    return ret;
                }
            }
            
            [fileManager copyItemAtPath:fromPath toPath:toPath error:&error];
            if (error) {
                MDZELog("Error saving to library: %@", error.localizedDescription);
                ret=false;
            } else {
                ret=true;
            }
        }
    }
    return ret;
}


@end

NS_ASSUME_NONNULL_END
