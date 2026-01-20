//
//  RadioSource.mm
//  modizer
//
//  Created by Yohann Magnien David on 10/01/2026.
//
#define RS_AMP_MAX_COMPOSER_ID 19822
#define RS_DOWNLOAD_MAX_RETRY_COUNT 16
#define RS_DOWNLOAD_WAIT 0.5 //wait time between 2 downloads, to avoid server overload

#define RS_MAX_DOWNLOAD 2
#define RS_QUEUE_SIZE 5
#define MAX_RS_HISTORY 64
#define MAX_RS_DUPLICATE_RETRY 1

#define RS_MODLAND_PLAYABLE_FILE_MAX_TRIES 32 //maximum nb of tries to find a playable file / Modland DB

#import "ModizerConstants.h"
#import "ModizFileHelper.h"
#import "ModizerTypes.h"
#import "RadioSource.h"
#import "TFHpple.h"

#include <stdlib.h>
#include <pthread.h>
extern pthread_mutex_t db_mutex;

extern volatile t_settings settings[MAX_SETTINGS];

extern const rsn_name_mapping_t SNESmusic_names[];
extern int snes_spc_entries;


NS_ASSUME_NONNULL_BEGIN

@implementation RadioSource

@synthesize mRadioSource,mPendingNewFileToPlay,mRadioSource_mode,mRetryCount,mRetryDuplCount;
@synthesize detailVC;
@synthesize mActive,mFilesList,mFilesExistInLibrary,mSourceData,mHistory,mCurrentPath;
@synthesize mURLSsession,mURLSessionQueue,mURLSessionConfig;

- (instancetype)init {
self = [super init];
if (self) {
mRadioSource=RS_NONE;
mRadioSource_mode=0;
mPendingNewFileToPlay=0;
mActive=NO;
mRetryCount=0;
mRetryDuplCount=0;
mFilesList = [NSMutableArray arrayWithCapacity:5];
mFilesExistInLibrary = [NSMutableArray arrayWithCapacity:5];
mSourceData = [NSMutableArray array];
mHistory = [NSMutableArray array];
mCurrentPath=nil;
[self cleanFiles];

mURLSessionConfig = [NSURLSessionConfiguration defaultSessionConfiguration];
mURLSessionConfig.HTTPMaximumConnectionsPerHost=1;

mURLSessionQueue = [[NSOperationQueue alloc] init];
mURLSessionQueue.maxConcurrentOperationCount = 1; // Séquentiel

mURLSsession = [NSURLSession sessionWithConfiguration:mURLSessionConfig
                                             delegate:self
                                        delegateQueue:mURLSessionQueue];
}
return self;
}

- (void)dealloc {
}

- (void)downloadFileFromURL:(NSString *)urlString rSource:(t_radioSource)rSource slot:(int)slot path:(NSString*)path filename:(NSString* __nullable)filename {
    NSURL *url = [NSURL URLWithString:[urlString stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]]];
    
    NSURLSessionDownloadTask *downloadTask = [mURLSsession downloadTaskWithURL:url
                                                             completionHandler:^(NSURL *location, NSURLResponse *response, NSError *error) {
        // Cast la réponse en NSHTTPURLResponse pour accéder au statusCode
        NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
        
        if (error) {
            // Erreur réseau ou autre
            MDZELog("Erreur de téléchargement: %@", error.localizedDescription);
            return;
        }
        
        if (httpResponse.statusCode == 404) {
            MDZELog("Fichier non trouvé (404)");
            // Traiter le cas 404
            //            mRetryDuplCount++;
            //            [self fetchNewFileFromSource:slot];
            return;
        }
        
        if (httpResponse.statusCode >= 400) {
            MDZELog("Erreur HTTP: %ld", (long)httpResponse.statusCode);
            // Traiter les autres erreurs HTTP
            return;
        }
        
        
        
        // Récupérer le nom de fichier suggéré par le serveur
        NSString *suggestedFilename = response.suggestedFilename;
        if (!suggestedFilename) {
            suggestedFilename = [url lastPathComponent];
        }
        
        if (filename) suggestedFilename=filename; //override
        
        NSString *collection=[self getSourceName:rSource];
        
        NSString *tmpName=[NSString stringWithFormat:@"%@/%@/%@",collection,path,suggestedFilename];
        bool duplicate=false;
        if ([mHistory containsObject:tmpName]) duplicate=true;
        
        if ((!duplicate) || (mRetryDuplCount>=MAX_RS_DUPLICATE_RETRY)) {
            mRetryDuplCount=0;
            
            // Destination finale
            NSString *destinationPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%d/%@/%@/%@",slot,collection,path,suggestedFilename];
            
            
            
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
                //MDZELog("Fichier téléchargé: %@", destinationPath);
                dispatch_async(dispatch_get_main_queue(), ^{
                    // Mise à jour UI si nécessaire
                    if ((mPendingNewFileToPlay==1)&&(slot==0)&&[ModizFileHelper isPlayableFile:destinationPath]) [self startNextEntry];
                });
                
            }
        } else {
            mRetryDuplCount++;
            [self fetchNewFileFromSource:slot];
        }
    }];
    
    [downloadTask resume];
}

-(NSMutableArray*) getASMA_DBEntries:(NSString*)dir1 dir2:(NSString* __nullable)dir2 dir3:(NSString* __nullable)dir3 {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    NSMutableArray *entries_arr=nil;
    int dbASMA_nb_entries;
    
    dbASMA_nb_entries=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        NSString *whereClause;
        //Build where clause
        whereClause=[NSString stringWithFormat:@"WHERE dir1=\"%@\"",dir1];
        if (dir2!=nil) {
            whereClause=[whereClause stringByAppendingFormat:@" AND dir2=\"%@\"",dir2];
        }
        if (dir3!=nil) {
            whereClause=[whereClause stringByAppendingFormat:@" AND dir3=\"%@\"",dir3];
        }
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //count how many entries we'll have
        snprintf(sqlStatement,1024,"SELECT COUNT(filename) FROM asma_file %s",[whereClause UTF8String]);
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                dbASMA_nb_entries+=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        if (dbASMA_nb_entries) {
            entries_arr=[NSMutableArray array];
            
            snprintf(sqlStatement,1024,"SELECT filename,fullpath,id_md5 FROM asma_file %s",[whereClause UTF8String]);
            
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    char *str=(char*)sqlite3_column_text(stmt, 0);
                    [entries_arr addObject:[NSString stringWithUTF8String:str]];
                    str=(char*)sqlite3_column_text(stmt, 1);
                    [entries_arr addObject:[NSString stringWithUTF8String:str]];
                }
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    return entries_arr;
}


-(void)getNewASMAFile:(int)slot {
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    NSError *err;
    
    //clean slot
    NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%d/",slot];
    [mFileMngr removeItemAtPath:localPath error:&err];
    //create tmp dir
    [mFileMngr createDirectoryAtPath:localPath withIntermediateDirectories:TRUE attributes:nil error:&err];
    [ModizFileHelper addSkipBackupAttributeToItemAtPath:localPath];
    
    int idx=arc4random_uniform((int)[mSourceData count]);
    NSString *str=[mSourceData objectAtIndex:idx];
    if ([[str substringToIndex:2] isEqualToString:@"f:"]) {
        //file
        str=[str substringFromIndex:2+1];
        NSString *localPath=[NSString stringWithFormat:@"%@/tmp/tmpRadio/%d/%@/%@",[ModizFileHelper getAppHomeDirectory],slot,ASMA_BASEDIR,str];
        NSString *asma_url=[NSString stringWithFormat:@"%s/%@",settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text,str ];
        
        [mFileMngr createDirectoryAtPath:[localPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&err];
        
        [self downloadFileFromURL:asma_url rSource:RS_COLLECTION_ASMA slot:slot path:[str stringByDeletingLastPathComponent] filename:nil];
    } else {
        //folder
        str=[str substringFromIndex:2];
        NSArray *tmp_arr=[str componentsSeparatedByString:@"/"];
        
        NSMutableArray *entries=nil;
        switch ([tmp_arr count]) {
            case 1:
                entries=[self getASMA_DBEntries:tmp_arr[0] dir2:nil dir3:nil];
                break;
            case 2:
                entries=[self getASMA_DBEntries:tmp_arr[0] dir2:tmp_arr[1] dir3:nil];
                break;
            case 3:
                entries=[self getASMA_DBEntries:tmp_arr[0] dir2:tmp_arr[1] dir3:tmp_arr[2]];
                break;
        }
        
        if (entries) {
            int fileIdx=arc4random_uniform((int)[entries count]/2);
            
            NSString *filename=[entries objectAtIndex:fileIdx*2];
            NSString *fullpath=[entries objectAtIndex:fileIdx*2+1];
            
            NSString *localPath=[NSString stringWithFormat:@"%@/tmp/tmpRadio/%d/%@%@",[ModizFileHelper getAppHomeDirectory],slot,ASMA_BASEDIR,fullpath];
            
            NSString *asma_url=[NSString stringWithFormat:@"%s%@",settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text,fullpath];
            
            [mFileMngr createDirectoryAtPath:[localPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&err];
            
            [self downloadFileFromURL:asma_url rSource:RS_COLLECTION_ASMA slot:slot path:[fullpath stringByDeletingLastPathComponent] filename:nil];
        } else {
            MDZILog("no entries from DB!");
        }
    }
}

-(NSMutableArray*) getCGSC_DBEntries:(NSString*)dir1 dir2:(NSString* __nullable)dir2 {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    NSMutableArray *entries_arr=nil;
    int dbCGSC_nb_entries;
    
    dbCGSC_nb_entries=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        NSString *whereClause;
        //Build where clause
        whereClause=[NSString stringWithFormat:@"WHERE dir1=\"%@\"",dir1];
        if (dir2!=nil) {
            whereClause=[whereClause stringByAppendingFormat:@" AND dir2=\"%@\"",dir2];
        }
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //count how many entries we'll have
        snprintf(sqlStatement,1024,"SELECT COUNT(filename) FROM cgsc_file %s",[whereClause UTF8String]);
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                dbCGSC_nb_entries+=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        if (dbCGSC_nb_entries) {
            entries_arr=[NSMutableArray array];
            
            snprintf(sqlStatement,1024,"SELECT filename,fullpath,id_md5 FROM cgsc_file %s",[whereClause UTF8String]);
            
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    char *str=(char*)sqlite3_column_text(stmt, 0);
                    [entries_arr addObject:[NSString stringWithUTF8String:str]];
                    str=(char*)sqlite3_column_text(stmt, 1);
                    [entries_arr addObject:[NSString stringWithUTF8String:str]];
                }
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    return entries_arr;
}


-(void)getNewCGSCFile:(int)slot {
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    NSError *err;
    NSString *cgsc_url=nil;
    NSString *localPath=nil;
    NSString *fullpath;
    
    //clean slot
    localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%d/",slot];
    [mFileMngr removeItemAtPath:localPath error:&err];
    //create tmp dir
    [mFileMngr createDirectoryAtPath:localPath withIntermediateDirectories:TRUE attributes:nil error:&err];
    [ModizFileHelper addSkipBackupAttributeToItemAtPath:localPath];
    
    int idx=arc4random_uniform((int)[mSourceData count]);
    NSString *str=[mSourceData objectAtIndex:idx];
    if ([[str substringToIndex:2] isEqualToString:@"f:"]) {
        //file
        str=[str substringFromIndex:2+1];
        localPath=[NSString stringWithFormat:@"%@/tmp/tmpRadio/%d/%@/%@",[ModizFileHelper getAppHomeDirectory],slot,CGSC_BASEDIR,str];
        cgsc_url=[NSString stringWithFormat:@"%s/%@",settings[ONLINE_CGSC_CURRENT_URL].detail.mdz_msgbox.text,str ];
        
        [mFileMngr createDirectoryAtPath:[localPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&err];
        
        fullpath=[NSString stringWithString:str];
    } else {
        //folder
        str=[str substringFromIndex:2];
        NSArray *tmp_arr=[str componentsSeparatedByString:@"/"];
        
        NSMutableArray *entries=nil;
        switch ([tmp_arr count]) {
            case 1:
                entries=[self getCGSC_DBEntries:tmp_arr[0] dir2:nil];
                break;
            case 2:
                entries=[self getCGSC_DBEntries:tmp_arr[0] dir2:tmp_arr[1]];
                break;
        }
        
        if (entries) {
            int fileIdx=arc4random_uniform((int)[entries count]/2);
            
            fullpath=[entries objectAtIndex:fileIdx*2+1];
            
            localPath=[NSString stringWithFormat:@"%@/tmp/tmpRadio/%d/%@%@",[ModizFileHelper getAppHomeDirectory],slot,CGSC_BASEDIR,fullpath];
            
            cgsc_url=[NSString stringWithFormat:@"%s%@",settings[ONLINE_CGSC_CURRENT_URL].detail.mdz_msgbox.text,fullpath];
            
            [mFileMngr createDirectoryAtPath:[localPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&err];
        }
    }
    
    if (cgsc_url) {
        NSMutableArray *addDataURL=[NSMutableArray array];
        NSMutableArray *addDataLocalPath=[NSMutableArray array];
        
        [addDataURL addObject:[[cgsc_url stringByDeletingPathExtension] stringByAppendingString:@".str"]];
        [addDataLocalPath addObject:[[fullpath stringByDeletingPathExtension] stringByAppendingString:@".str"]];
        [addDataURL addObject:[[cgsc_url stringByDeletingPathExtension] stringByAppendingString:@".wds"]];
        [addDataLocalPath addObject:[[fullpath stringByDeletingPathExtension] stringByAppendingString:@".wds"]];
        [addDataURL addObject:[[cgsc_url stringByDeletingPathExtension] stringByAppendingString:@".pic"]];
        [addDataLocalPath addObject:[[fullpath stringByDeletingPathExtension] stringByAppendingString:@".pic"]];
        [addDataURL addObject:[[cgsc_url stringByDeletingPathExtension] stringByAppendingString:@".pgg"]];
        [addDataLocalPath addObject:[[fullpath stringByDeletingPathExtension] stringByAppendingString:@".pgg"]];
        [addDataURL addObject:[[cgsc_url stringByDeletingPathExtension] stringByAppendingString:@".pjj"]];
        [addDataLocalPath addObject:[[fullpath stringByDeletingPathExtension] stringByAppendingString:@".pjj"]];
        
        for (int i=0;i<[addDataURL count];i++) {
            [self downloadFileFromURL:[addDataURL objectAtIndex:i] rSource:RS_COLLECTION_CGSC slot:slot path:[[addDataLocalPath objectAtIndex:i] stringByDeletingLastPathComponent] filename:nil];
        }
        
        [self downloadFileFromURL:cgsc_url rSource:RS_COLLECTION_CGSC slot:slot path:[fullpath stringByDeletingLastPathComponent] filename:nil];
    } else {
        MDZILog("no entries from DB!");
    }
}

-(NSMutableArray*) getHVSC_DBEntries:(NSString*)dir1 dir2:(NSString* __nullable)dir2 dir3:(NSString* __nullable)dir3 dir4:(NSString* __nullable)dir4 dir5:(NSString* __nullable)dir5 {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    NSMutableArray *entries_arr=nil;
    int dbHVSC_nb_entries;
    
    dbHVSC_nb_entries=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        NSString *whereClause;
        //Build where clause
        whereClause=[NSString stringWithFormat:@"WHERE dir1=\"%@\"",dir1];
        if (dir2!=nil) {
            whereClause=[whereClause stringByAppendingFormat:@" AND dir2=\"%@\"",dir2];
        }
        if (dir3!=nil) {
            whereClause=[whereClause stringByAppendingFormat:@" AND dir3=\"%@\"",dir3];
        }
        if (dir4!=nil) {
            whereClause=[whereClause stringByAppendingFormat:@" AND dir4=\"%@\"",dir4];
        }
        if (dir5!=nil) {
            whereClause=[whereClause stringByAppendingFormat:@" AND dir5=\"%@\"",dir5];
        }
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //count how many entries we'll have
        snprintf(sqlStatement,1024,"SELECT COUNT(filename) FROM hvsc_file %s",[whereClause UTF8String]);
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                dbHVSC_nb_entries+=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        if (dbHVSC_nb_entries) {
            entries_arr=[NSMutableArray array];
            
            snprintf(sqlStatement,1024,"SELECT filename,fullpath,id_md5 FROM hvsc_file %s",[whereClause UTF8String]);
            
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    char *str=(char*)sqlite3_column_text(stmt, 0);
                    [entries_arr addObject:[NSString stringWithUTF8String:str]];
                    str=(char*)sqlite3_column_text(stmt, 1);
                    [entries_arr addObject:[NSString stringWithUTF8String:str]];
                }
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    return entries_arr;
}


-(void)getNewHVSCFile:(int)slot {
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    NSError *err;
    
    //clean slot
    NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%d/",slot];
    [mFileMngr removeItemAtPath:localPath error:&err];
    //create tmp dir
    [mFileMngr createDirectoryAtPath:localPath withIntermediateDirectories:TRUE attributes:nil error:&err];
    [ModizFileHelper addSkipBackupAttributeToItemAtPath:localPath];
    
    int idx=arc4random_uniform((int)[mSourceData count]);
    NSString *str=[mSourceData objectAtIndex:idx];
    if ([[str substringToIndex:2] isEqualToString:@"f:"]) {
        //file
        str=[str substringFromIndex:2+1];
        NSString *localPath=[NSString stringWithFormat:@"%@/tmp/tmpRadio/%d/%@/%@",[ModizFileHelper getAppHomeDirectory],slot,HVSC_BASEDIR,str];
        NSString *hvsc_url=[NSString stringWithFormat:@"%s/%@",settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text,str ];
        
        [mFileMngr createDirectoryAtPath:[localPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&err];
        
        [self downloadFileFromURL:hvsc_url rSource:RS_COLLECTION_HVSC slot:slot path:[str stringByDeletingLastPathComponent] filename:nil];
    } else {
        //folder
        str=[str substringFromIndex:2];
        NSArray *tmp_arr=[str componentsSeparatedByString:@"/"];
        
        NSMutableArray *entries=nil;
        switch ([tmp_arr count]) {
            case 1:
                entries=[self getHVSC_DBEntries:tmp_arr[0] dir2:nil dir3:nil dir4:nil dir5:nil];
                break;
            case 2:
                entries=[self getHVSC_DBEntries:tmp_arr[0] dir2:tmp_arr[1] dir3:nil dir4:nil dir5:nil];
                break;
            case 3:
                entries=[self getHVSC_DBEntries:tmp_arr[0] dir2:tmp_arr[1] dir3:tmp_arr[2] dir4:nil dir5:nil];
                break;
            case 4:
                entries=[self getHVSC_DBEntries:tmp_arr[0] dir2:tmp_arr[1] dir3:tmp_arr[2] dir4:tmp_arr[3] dir5:nil];
                break;
            case 5:
                entries=[self getHVSC_DBEntries:tmp_arr[0] dir2:tmp_arr[1] dir3:tmp_arr[2] dir4:tmp_arr[3] dir5:tmp_arr[3]];
                break;
        }
        
        if (entries) {
            int fileIdx=arc4random_uniform((int)[entries count]/2);
            
            NSString *fullpath=[entries objectAtIndex:fileIdx*2+1];
            
            NSString *localPath=[NSString stringWithFormat:@"%@/tmp/tmpRadio/%d/%@%@",[ModizFileHelper getAppHomeDirectory],slot,HVSC_BASEDIR,fullpath];
            
            NSString *hvsc_url=[NSString stringWithFormat:@"%s%@",settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text,fullpath];
            
            [mFileMngr createDirectoryAtPath:[localPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&err];
            
            [self downloadFileFromURL:hvsc_url rSource:RS_COLLECTION_HVSC slot:slot path:[fullpath stringByDeletingLastPathComponent] filename:nil];
        } else {
            MDZILog("no entries from DB!");
        }
    }
}

-(NSString*) getMODLANDLocalForRemote:(NSString*)remotePath {
    NSArray *arr=[remotePath componentsSeparatedByString:@"/"];
    NSString *res=[arr[1] stringByAppendingFormat:@"/%@",arr[0]];
    for (int i=2;i<[arr count];i++)
        res=[res stringByAppendingFormat:@"/%@",arr[i]];
    return res;
}

-(NSString*) getMODLAND_localPath:(int)id_mod {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    NSString *fullpath=nil;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        
        snprintf(sqlStatement,1024,"select fullpath from mod_file where id=%d",id_mod);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                fullpath=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
    }
    
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return fullpath;
}

-(int) getMODLAND_MaxModID {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int dbMODLAND_max_id;
    
    dbMODLAND_max_id=-1;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        //Build where clause
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //count how many entries we'll have
        snprintf(sqlStatement,1024,"SELECT max(id) FROM mod_file");
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                dbMODLAND_max_id=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    return dbMODLAND_max_id;
}

-(NSMutableArray*) getMODLAND_DBEntries:(int)id_type id_author:(int)id_author id_album:(int)id_album  {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    NSMutableArray *entries_arr=nil;
    int dbASMA_nb_entries;
    
    dbASMA_nb_entries=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        NSString *whereClause=nil;
        //Build where clause
        
        if (id_type>=0) {
            if (whereClause) whereClause=[whereClause stringByAppendingFormat:@" AND id_type=%d",id_type];
            else whereClause=[NSString stringWithFormat:@"WHERE id_type=%d",id_type];
        }
        if (id_author>=0) {
            if (whereClause) whereClause=[whereClause stringByAppendingFormat:@" AND id_author=%d",id_author];
            else whereClause=[NSString stringWithFormat:@"WHERE id_author=%d",id_author];
        }
        if (id_album>=0) {
            if (whereClause) whereClause=[whereClause stringByAppendingFormat:@" AND id_album=%d",id_album];
            else whereClause=[NSString stringWithFormat:@"WHERE id_album=%d",id_album];
        }
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //count how many entries we'll have
        snprintf(sqlStatement,1024,"SELECT COUNT(id) FROM mod_file %s",[whereClause UTF8String]);
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                dbASMA_nb_entries+=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        if (dbASMA_nb_entries) {
            entries_arr=[NSMutableArray array];
            
            snprintf(sqlStatement,1024,"SELECT id FROM mod_file %s",[whereClause UTF8String]);
            
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    int id_mod=sqlite3_column_int(stmt,0);
                    [entries_arr addObject:[NSNumber numberWithInt:id_mod]];
                }
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    return entries_arr;
}


-(void)getNewMODLANDFile:(int)slot {
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    NSError *err;
    NSString *str;
    NSString *localPath;
    NSString *modland_url;
    
    //clean slot
    localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%d/",slot];
    [mFileMngr removeItemAtPath:localPath error:&err];
    //create tmp dir
    [mFileMngr createDirectoryAtPath:localPath withIntermediateDirectories:TRUE attributes:nil error:&err];
    [ModizFileHelper addSkipBackupAttributeToItemAtPath:localPath];
    
    int id_mod=-1;
    if ([mSourceData count]>0) {
        int idx=arc4random_uniform((int)[mSourceData count]);
        str=[mSourceData objectAtIndex:idx];
        if ([[str substringToIndex:2] isEqualToString:@"f:"]) {
            //file
            str=[str substringFromIndex:2];
            
            id_mod=atoi([str UTF8String]);
        } else {
            //folder
            str=[str substringFromIndex:2];
            NSArray *tmp_arr=[str componentsSeparatedByString:@"/"];
            
            NSMutableArray *entries=nil;
            entries=[self getMODLAND_DBEntries:atoi([tmp_arr[0] UTF8String]) id_author:atoi([tmp_arr[1] UTF8String]) id_album:atoi([tmp_arr[2] UTF8String])];
            
            if (entries) {
                int fileIdx=arc4random_uniform((int)[entries count]);
                
                int max_tries=RS_MODLAND_PLAYABLE_FILE_MAX_TRIES;
                while (max_tries) {
                    id_mod=[[entries objectAtIndex:fileIdx] intValue];
                    str=[self getMODLAND_localPath:id_mod];
                    if ([ModizFileHelper isPlayableFile:str]) break;
                    max_tries--;
                }
            }
        }
    } else {
        int max_mod_id=[self getMODLAND_MaxModID];
        int max_tries=RS_MODLAND_PLAYABLE_FILE_MAX_TRIES;
        while (max_tries) {
            id_mod=arc4random_uniform(max_mod_id)+1;
            str=[self getMODLAND_localPath:id_mod];
            if ([ModizFileHelper isPlayableFile:str]) break;
            max_tries--;
        }
    }
    if (id_mod>=0) {
        str=[self getMODLAND_localPath:id_mod];
        MDZILog("str %@",str);
        NSArray *addFiles=[ModizFileHelper getAdditionalMODLANDRequiredFiles:str];
        for (NSString *addFile in addFiles) {
            MDZILog("2download: %@",addFile);
            
            localPath=[NSString stringWithFormat:@"%@/tmp/tmpRadio/%d/%@/%@",[ModizFileHelper getAppHomeDirectory],slot,MODLAND_BASEDIR,[self getMODLANDLocalForRemote:addFile]];
            modland_url=[NSString stringWithFormat:@"%s/pub/modules/%@",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text,addFile ];
            [mFileMngr createDirectoryAtPath:[localPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&err];
            [self downloadFileFromURL:modland_url rSource:RS_COLLECTION_MODLAND slot:slot path:[[self getMODLANDLocalForRemote:addFile] stringByDeletingLastPathComponent] filename:nil];
        }
        //main file to download
        localPath=[NSString stringWithFormat:@"%@/tmp/tmpRadio/%d/%@/%@",[ModizFileHelper getAppHomeDirectory],slot,MODLAND_BASEDIR,[self getMODLANDLocalForRemote:str]];
        modland_url=[NSString stringWithFormat:@"%s/pub/modules/%@",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text,str ];
        
        [mFileMngr createDirectoryAtPath:[localPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&err];
        
        //        MDZILog("download: %@",modland_url);
        [self downloadFileFromURL:modland_url rSource:RS_COLLECTION_MODLAND slot:slot path:[[self getMODLANDLocalForRemote:str] stringByDeletingLastPathComponent] filename:nil];
    } else {
        MDZILog("no entries from DB!");
    }
}

-(NSString*) getSNESMshortnameFromID:(int)entry_id  {
    NSString *res=nil;
    TFHppleElement *el;
    NSString *urlEntry=[NSString stringWithFormat:@"http://snesmusic.org/v2/profile.php?profile=set&selected=%d",entry_id];
    
    NSData *urlDataEntry = [NSData dataWithContentsOfURL:[NSURL URLWithString:[NSString stringWithString:urlEntry]]];
    TFHpple *docEntry       = [[TFHpple alloc] initWithHTMLData:urlDataEntry];
    
    //NSArray *arr_entry_name=[docEntry searchWithXPathQuery:@"/html/body/div[@id='contContainer']/h2[1]/text()"];
    NSArray *arr_entry_file=[docEntry searchWithXPathQuery:@"/html/body//a[@class='download']"];
    
    if (!arr_entry_file) return nil;
    if (![arr_entry_file count]) return nil;
    
    el=[arr_entry_file firstObject];
    res=[[[el objectForKey:@"href"] componentsSeparatedByString:@"="] lastObject];
    
    return res;
}


-(void)getNewSNESFile:(int)slot {
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    NSError *err;
    
    //clean slot
    NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%d/",slot];
    [mFileMngr removeItemAtPath:localPath error:&err];
    //create tmp dir
    [mFileMngr createDirectoryAtPath:localPath withIntermediateDirectories:TRUE attributes:nil error:&err];
    [ModizFileHelper addSkipBackupAttributeToItemAtPath:localPath];
    
    int idx=arc4random_uniform((int)[mSourceData count]);
    NSString *str=[mSourceData objectAtIndex:idx];
    if ([[str substringToIndex:2] isEqualToString:@"f:"]) {
        //file
        str=[str substringFromIndex:2];
        NSArray *str_arr=[str componentsSeparatedByString:@"/"];
        NSString *localPath=[NSString stringWithFormat:@"%@/tmp/tmpRadio/%d/%@/%@",[ModizFileHelper getAppHomeDirectory],slot,SNESmusic_BASEDIR,str_arr[0]];
        
        //create folder if needed
        [mFileMngr createDirectoryAtPath:[localPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&err];
        
        //get screenshot
        NSString *snes_url=[NSString stringWithFormat:@"http://snesmusic.org/v2/images/screenshots/%@.png",str ];
        [self downloadFileFromURL:snes_url rSource:RS_COLLECTION_SNES slot:slot path:[str stringByDeletingLastPathComponent] filename:[str_arr[1] stringByAppendingString:@".png"]];

        //get rsn file
        snes_url=[NSString stringWithFormat:@"http://snesmusic.org/v2/download.php?spcNow=%@",str ];
        
        [self downloadFileFromURL:snes_url rSource:RS_COLLECTION_SNES slot:slot path:[str stringByDeletingLastPathComponent]filename:[str_arr[1] stringByAppendingString:@".rsn"]];
    } else if ([[str substringToIndex:2] isEqualToString:@"i:"]) {
        //file
        str=[str substringFromIndex:2];
        
        int entry_id=[str intValue];
        str=[self getSNESMshortnameFromID:entry_id];
        
        NSString *str_long=str; //by default use shortname
        for (int i=0;i<snes_spc_entries;i++) {
            if (strcmp(SNESmusic_names[i].shortname,[str UTF8String])==0) {
                str_long=[NSString stringWithUTF8String:SNESmusic_names[i].longname];
                break;
            }
        }
        
        NSString *localPath=[NSString stringWithFormat:@"%@/tmp/tmpRadio/%d/%@/%@",[ModizFileHelper getAppHomeDirectory],slot,SNESmusic_BASEDIR,str];
        
        //create folder if needed
        [mFileMngr createDirectoryAtPath:[localPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&err];
        
        //get screenshot
        NSString *snes_url=[NSString stringWithFormat:@"http://snesmusic.org/v2/images/screenshots/%@.png",str ];
        [self downloadFileFromURL:snes_url rSource:RS_COLLECTION_SNES slot:slot path:[str stringByDeletingLastPathComponent]filename:[str_long stringByAppendingString:@".png"]];

        //get rsn file
        snes_url=[NSString stringWithFormat:@"http://snesmusic.org/v2/download.php?spcNow=%@",str ];
        
        [self downloadFileFromURL:snes_url rSource:RS_COLLECTION_SNES slot:slot path:[str stringByDeletingLastPathComponent]filename:[str_long stringByAppendingString:@".rsn"]];
    }
    
    
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
        //All or composer list
        //
        
        int mAMP_max_compID=RS_AMP_MAX_COMPOSER_ID;
        int composer_id=arc4random_uniform(mAMP_max_compID)+1;
        
        if (mRadioSource_mode==1) {
            if ([mSourceData count]) {
                int idx=arc4random_uniform((int)[mSourceData count]);
                composer_id=atoi([mSourceData[idx] UTF8String]);
            }
            else {
                MDZELog("Radio: source is AMP, mode is 1 and no composer ID is available. using random value");
            }
        }
        fileURL=[NSString stringWithFormat:@"https://amp.dascene.net/detail.php?detail=modules&view=%d",composer_id];
        
        NSURL *url = [NSURL URLWithString:[[NSString stringWithFormat:@"%@",fileURL] stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]]];
        
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
                //                MDZILog("composer: %@",composer);
                
                el=[arr_url_modulesNb objectAtIndex:0];
                int mod_nb=atoi([el.content UTF8String]);
                //                MDZILog("modules: %d",mod_nb);
                
                mod_id=arc4random_uniform(mod_nb);
                el=[arr_url_fileList objectAtIndex:mod_id];
                NSString *fileModURL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el objectForKey:@"href"]];
                
                if ([composer length] && [fileModURL length]) {
                    mRetryCount=0;
                    [self downloadFileFromURL:fileModURL rSource:RS_COLLECTION_AMP slot:slot path:composer filename:nil];
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
    } else if (mRadioSource_mode==2) {
        //Mods list
        if ([mSourceData count]) {
            int idx=arc4random_uniform((int)[mSourceData count]/2)*2;
            
            fileURL=[mSourceData objectAtIndex:idx];
            NSArray *tmpAA=[[mSourceData objectAtIndex:idx+1] componentsSeparatedByString:@"/"];
            NSString *composer=@"unknown";
            if ([tmpAA count]>=4) composer=tmpAA[2];
            
            [self downloadFileFromURL:fileURL rSource:RS_COLLECTION_AMP slot:slot path:composer filename:nil];
        }
    }  else if (mRadioSource_mode==3) {
        //Groups
        if ([mSourceData count]) {
            //select a group
            int idx=arc4random_uniform((int)[mSourceData count]);
            
            NSURL *url = [NSURL URLWithString:[[NSString stringWithFormat:@"%@",mSourceData[idx]] stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]]];
            
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
                
                //get handle list
                NSArray *arr_url_handleList=[doc searchWithXPathQuery:@"//td[@class='descript' and contains(normalize-space(.), 'Handle:')]/following-sibling::td[1]/a"];
                
                if ((arr_url_handleList==nil)||([arr_url_handleList count]==0)) return;
                
                int compIdx=arc4random_uniform((int)[arr_url_handleList count]);
                
                TFHppleElement *el=[arr_url_handleList objectAtIndex:compIdx];
                NSString *modListURL=[NSString stringWithFormat:@"https://amp.dascene.net/%@&detail=modules",[el objectForKey:@"href"]];
                
                NSURL *urlMods = [NSURL URLWithString:[[NSString stringWithFormat:@"%@",modListURL] stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]]];
                
                NSURLSession *sessionMods = [NSURLSession sharedSession];
                
                NSURLSessionDataTask *taskMod =
                [sessionMods dataTaskWithURL:urlMods
                           completionHandler:^(NSData * _Nullable dataMod,
                                               NSURLResponse * _Nullable responseMod,
                                               NSError * _Nullable errorMod) {
                    if (errorMod) {
                        MDZELog("Erreur réseau : %@", errorMod);
                        return;
                    }
                    
                    if (!dataMod) {
                        MDZELog("Aucune donnée reçue");
                        return;
                    }
                    
                    TFHpple *docMod       = [[TFHpple alloc] initWithHTMLData:dataMod];
                    
                    NSArray *arr_url_composerList=[docMod searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[2]"];
                    
                    NSArray *arr_url_modulesNb=[docMod searchWithXPathQuery:@"//td[@class='descript' and contains(normalize-space(.), 'Modules')]/following-sibling::td[1]/a"];
                    NSArray *arr_url_fileList=[docMod searchWithXPathQuery:@"//div[@id='result']//tr[@class='tr0' or @class='tr1']/td[1]/a"];
                    
                    arr_url_composerList=[arr_url_composerList subarrayWithRange:NSMakeRange([arr_url_composerList count]-[arr_url_fileList count], [arr_url_fileList count])];
                    
                    TFHppleElement *el;
                    int mod_id=-1;
                    //add entries for each group & country
                    if ([arr_url_modulesNb count]>0) {
                        
                        el=[arr_url_composerList objectAtIndex:0];
                        NSString *composer=[NSString stringWithString:el.content];
                        //                MDZILog("composer: %@",composer);
                        
                        el=[arr_url_modulesNb objectAtIndex:0];
                        int mod_nb=atoi([el.content UTF8String]);
                        //                MDZILog("modules: %d",mod_nb);
                        
                        mod_id=arc4random_uniform(mod_nb);
                        el=[arr_url_fileList objectAtIndex:mod_id];
                        NSString *fileModURL=[NSString stringWithFormat:@"https://amp.dascene.net/%@",[el objectForKey:@"href"]];
                        
                        if ([composer length] && [fileModURL length]) {
                            mRetryCount=0;
                            [self downloadFileFromURL:fileModURL rSource:RS_COLLECTION_AMP slot:slot path:composer filename:nil];
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
                
                [taskMod resume];
            }];
            
            [task resume];
        }
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
        
        if (![fullPath containsString:@"tmpRadio/History/"]) {
            
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
        case RS_COLLECTION_ASMA:
            [self getNewASMAFile:slot];
            break;
        case RS_COLLECTION_CGSC:
            [self getNewCGSCFile:slot];
            break;
        case RS_COLLECTION_HVSC:
            [self getNewHVSCFile:slot];
            break;
        case RS_COLLECTION_MODLAND:
            [self getNewMODLANDFile:slot];
            break;
        case RS_COLLECTION_SNES:
            [self getNewSNESFile:slot];
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
        
        mCurrentPath=[NSString stringWithString:localPath];
        [detailVC play_listmodules:array_label start_index:0 path:array_path];
    }
}

-(void) removeLastHistoryItem {
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    NSError *error;
    
    NSString *histoPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/History"];
    NSString *targetPath=[NSString stringWithFormat:@"%@/0",histoPath];
    //remove the first dir
    [mFileMngr removeItemAtPath:targetPath error:NULL];
    
    //move the other ones accordingly
    for (int i=1;i<[mHistory count];i++) {
        NSString *sourcePath=[NSString stringWithFormat:@"%@/%d",histoPath,i];
        targetPath=[NSString stringWithFormat:@"%@/%d",histoPath,i-1];
        
        [mFileMngr moveItemAtPath:sourcePath toPath:targetPath error:&error];
        if (error) {
            MDZELog("Error moving for histo update from slot %d to %d : %@",i,i-1,error.localizedDescription);
        }
    }
    
    [mHistory removeLastObject];
}

-(void)fetchRenewFilesAndStart:(bool)removeCurrentEntry {
    //update files list
    [self scanForPlayableFiles];
    
    if ([mFilesList count]) {
        NSFileManager *mFileMngr=[[NSFileManager alloc] init];
        NSError *error;
        
        //move or delete 1st one to history
        NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/0"];
        
        int hist_idx=(int)[mHistory count];

        //create histo dir if not already created
        NSString *histoPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/History"];
        [mFileMngr createDirectoryAtPath:histoPath withIntermediateDirectories:TRUE attributes:nil error:NULL];

        if (removeCurrentEntry) {
            //Do not archive, remove item
            [mFileMngr removeItemAtPath:localPath error:NULL];
        } else {
            //Archive, move item
            NSString *targetPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/History/%d",hist_idx];
            [mFileMngr removeItemAtPath:targetPath error:NULL];
            [mFileMngr moveItemAtPath:localPath toPath:targetPath error:&error];
            if (error) {
                MDZELog("Error moving for histo : %@",error.localizedDescription);
            }
            
            NSString *tmpName=mFilesList[0];
            [mHistory insertObject:[tmpName substringFromIndex:[tmpName rangeOfString:@"/"].location+1] atIndex:0];
            if ([mHistory count]>MAX_RS_HISTORY) {
                [self removeLastHistoryItem];
            }
        }
        
        [mFilesList removeObjectAtIndex:0];
        [mFilesExistInLibrary removeObjectAtIndex:0];
        //rename others
        for (int i=0;i<[mFilesList count];i++) {
            NSString *fromPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%@",[[[mFilesList objectAtIndex:i] componentsSeparatedByString:@"/"] firstObject]];
            NSString *toPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%d",i];
            //            MDZILog("move from %@\nto %@",fromPath,toPath);
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
    
    int max_download=RS_MAX_DOWNLOAD;
    for (int i=(int)[mFilesList count];i<RS_QUEUE_SIZE;i++) {
        //initiate a new download
        [self fetchNewFileFromSource:i];
        max_download--;
        if (!max_download) break;
        usleep(1000*1000*RS_DOWNLOAD_WAIT); //wait
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
        case RS_COLLECTION_SNES:
            return @"SNESM";
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
        case RS_COLLECTION_SNES:
            return @"SNESmusic";
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
    [mHistory removeAllObjects];
}

-(void) movePrev:(int)idx {
    if (idx>=[mHistory count]) return;
    NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/History/%d/%@",(int)[mHistory count]-1-idx,mHistory[idx]];
    
    NSFileManager *mFileMngr = [[NSFileManager alloc] init];
    if (![mFileMngr fileExistsAtPath:localPath]) return;
    
    NSMutableArray *array_label = [[NSMutableArray alloc] init];
    NSMutableArray *array_path = [[NSMutableArray alloc] init];
    
    [array_label addObject:mHistory[idx]];
    
    [array_path addObject:localPath];
    
    mCurrentPath=[NSString stringWithString:localPath];
    [detailVC play_listmodules:array_label start_index:0 path:array_path];
}

-(void) startCurrent {
    if ([mFilesList count]) {
        NSMutableArray *array_label = [[NSMutableArray alloc] init];
        NSMutableArray *array_path = [[NSMutableArray alloc] init];
        
        [array_label addObject:mFilesList[0]];
        
        NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/tmp/tmpRadio/%@",mFilesList[0]];
        [array_path addObject:localPath];
        
        mCurrentPath=[NSString stringWithString:localPath];
        [detailVC play_listmodules:array_label start_index:0 path:array_path];
    }
}


-(void) moveNext:(bool)removeCurrentEntry {
    static int no_reenter=0;
    if (no_reenter) return;
    no_reenter=1;
    
    [self fetchRenewFilesAndStart:removeCurrentEntry];
    no_reenter=0;
}

-(void) activate {
    if (![mSourceData count]) return;
    mActive=YES;
    [self moveNext:FALSE];
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
        int arr_count=(int)[arr count];
        if (mRadioSource==RS_COLLECTION_AMP) {
            if (arr_count>=4) result=[NSString stringWithFormat:@"%@ by %@ (%@)",[arr[arr_count-1] stringByDeletingPathExtension],arr[arr_count-2],arr[1]];
            else result=[NSString stringWithFormat:@"%@ (%@)",[[arr lastObject] stringByDeletingPathExtension],arr[1]];
        }
        if (mRadioSource==RS_COLLECTION_ASMA) {
            if ((arr_count>=4)&&([[arr objectAtIndex:2] isEqualToString:@"Composers"])) result=[NSString stringWithFormat:@"%@ by %@ (%@)",[arr[arr_count-1] stringByDeletingPathExtension],arr[3],arr[1]];
            else if ((arr_count>=4)&&([[arr objectAtIndex:2] isEqualToString:@"Groups"])) result=[NSString stringWithFormat:@"%@ by %@ (%@)",[arr[arr_count-1] stringByDeletingPathExtension],arr[3],arr[1]];
            else result=[NSString stringWithFormat:@"%@ (%@)",[[arr lastObject] stringByDeletingPathExtension],arr[1]];
        }
        if (mRadioSource==RS_COLLECTION_CGSC) {
            result=[NSString stringWithFormat:@"%@ by %@ (%@)",[arr[arr_count-1] stringByDeletingPathExtension],arr[2],arr[1]];
        }
        if (mRadioSource==RS_COLLECTION_HVSC) {
            if ((arr_count>=4)&&([[arr objectAtIndex:2] isEqualToString:@"MUSICIANS"])) result=[NSString stringWithFormat:@"%@ by %@ (%@)",[arr[arr_count-1] stringByDeletingPathExtension],arr[4],arr[1]];
            else result=[NSString stringWithFormat:@"%@ (%@)",[[arr lastObject] stringByDeletingPathExtension],arr[1]];
        }
        if (mRadioSource==RS_COLLECTION_MODLAND) {
            result=[NSString stringWithFormat:@"%@ by %@ (%@)",[arr[arr_count-1] stringByDeletingPathExtension],arr[2],arr[1]];
        }
        if (mRadioSource==RS_COLLECTION_SNES) {
            result=[NSString stringWithFormat:@"%@ (%@)",[arr[arr_count-1] stringByDeletingPathExtension],arr[1]];
        }
    }
    return result;
}

-(int) getHistorySize {
    return (int)[mHistory count];
}

-(NSString *) getHistoryLabel:(int)idx {
    NSString *result=@"";
    
    int max_hist=(int)[mHistory count];
    if (idx>=max_hist) return @"";
    
    NSArray *arr=[mHistory[idx] componentsSeparatedByString:@"/"];
    int arr_count=(int)[arr count];
    if (mRadioSource==RS_COLLECTION_AMP) {
        if (arr_count>=3) result=[result stringByAppendingFormat:@"%@ by %@ (%@)\n",[arr[arr_count-1] stringByDeletingPathExtension],arr[1],arr[0]];
        else result=[result stringByAppendingFormat:@"%@\n",[[arr lastObject] stringByDeletingPathExtension]];
    }
    if (mRadioSource==RS_COLLECTION_ASMA) {
        if ((arr_count>=3)&&([[arr objectAtIndex:1] isEqualToString:@"Composers"])) result=[result stringByAppendingFormat:@"%@ by %@ (%@)\n",[arr[arr_count-1] stringByDeletingPathExtension],arr[2],arr[0]];
        else if ((arr_count>=3)&&([[arr objectAtIndex:1] isEqualToString:@"Groups"])) result=[result stringByAppendingFormat:@"%@ by %@ (%@)\n",[arr[arr_count-1] stringByDeletingPathExtension],arr[2],arr[0]];
        else result=[result stringByAppendingFormat:@"%@ (%@)\n",[[arr lastObject] stringByDeletingPathExtension],arr[0]];
    }
    if (mRadioSource==RS_COLLECTION_CGSC) {
        result=[result stringByAppendingFormat:@"%@ by %@ (%@)\n",[arr[arr_count-1] stringByDeletingPathExtension],arr[1],arr[0]];
    }
    if (mRadioSource==RS_COLLECTION_HVSC) {
        if ((arr_count>=3)&&([[arr objectAtIndex:1] isEqualToString:@"MUSICIANS"])) result=[result stringByAppendingFormat:@"%@ by %@ (%@)\n",[arr[arr_count-1] stringByDeletingPathExtension],arr[3],arr[0]];
        else result=[result stringByAppendingFormat:@"%@ (%@)\n",[[arr lastObject] stringByDeletingPathExtension],arr[0]];
    }
    if (mRadioSource==RS_COLLECTION_MODLAND) {
        result=[result stringByAppendingFormat:@"%@ by %@ (%@)\n",[arr[arr_count-1] stringByDeletingPathExtension],arr[1],arr[0]];
    }
    if (mRadioSource==RS_COLLECTION_SNES) {
        result=[result stringByAppendingFormat:@"%@ (%@)\n",[arr[arr_count-1] stringByDeletingPathExtension],arr[0]];
    }
    return result;
}


-(bool) saveFileToLibrary:(NSString*  __nullable)suggestedName {
    bool ret=true;
    [self scanForPlayableFiles];
    if (mCurrentPath) {
        NSString *relativePath;
        if ([mCurrentPath containsString:@"tmpRadio/History/"]) {
            relativePath=[mCurrentPath substringFromIndex:[mCurrentPath rangeOfString:@"tmpRadio/History/"].location+[@"tmpRadio/History/" length]];
        } else {
            relativePath=[mCurrentPath substringFromIndex:[mCurrentPath rangeOfString:@"tmpRadio/"].location+[@"tmpRadio/" length]];
        }
        
//        if (mRadioSource==RS_COLLECTION_SNES) {
//            //try to get game name
//            MDZILog("%@",suggestedName);
//            
//            NSFileManager *fileManager = [[NSFileManager alloc] init];
//            NSError *error=nil;
//            NSString *fromPath = [mCurrentPath stringByDeletingLastPathComponent];
//            
//            NSString *toPath = [[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/Documents/%@",[relativePath substringFromIndex:[relativePath rangeOfString:@"/"].location+1]] stringByDeletingLastPathComponent];
//            //MDZILog("from %@\nto %@",fromPath,toPath);
//            // Crée les dossiers intermédiaires si besoin
//            if (![fileManager fileExistsAtPath:toPath]) {
//                BOOL created = [fileManager createDirectoryAtPath:toPath
//                                       withIntermediateDirectories:YES
//                                                        attributes:nil
//                                                             error:&error];
//                if (!created) {
//                    MDZELog("Erreur création dossier: %@", error);
//                    ret=false;
//                    return ret;
//                }
//            }
//            
//            //[fileManager copyItemAtPath:fromPath toPath:toPath error:&error];
//            NSArray *contents = [fileManager contentsOfDirectoryAtPath:fromPath error:&error];
//            if (error) {
//                MDZELog("Error saving to library: %@", error.localizedDescription);
//                ret=false;
//            }
//            for (NSString *item in contents) {
//                NSString *sourcePath = [fromPath stringByAppendingPathComponent:item];
//                NSString *destPath = [[toPath stringByAppendingPathComponent:suggestedName] stringByAppendingFormat:@".%@",[sourcePath pathExtension]];
//                [fileManager copyItemAtPath:sourcePath toPath:destPath error:&error];
//                if (error) {
//                    MDZELog("Error saving to library: %@", error.localizedDescription);
//                    ret=false;
//                }
//            }
//        } else
        if ( (mRadioSource==RS_COLLECTION_AMP) || (mRadioSource==RS_COLLECTION_ASMA) ||
             (mRadioSource==RS_COLLECTION_CGSC) || (mRadioSource==RS_COLLECTION_HVSC) ||
             (mRadioSource==RS_COLLECTION_MODLAND) || (mRadioSource==RS_COLLECTION_SNES) ) {
            NSFileManager *fileManager = [[NSFileManager alloc] init];
            NSError *error=nil;
            NSString *fromPath = [mCurrentPath stringByDeletingLastPathComponent];
            
            NSString *toPath = [[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/Documents/%@",[relativePath substringFromIndex:[relativePath rangeOfString:@"/"].location+1]] stringByDeletingLastPathComponent];
            //MDZILog("from %@\nto %@",fromPath,toPath);
            // Crée les dossiers intermédiaires si besoin
            if (![fileManager fileExistsAtPath:toPath]) {
                BOOL created = [fileManager createDirectoryAtPath:toPath
                                       withIntermediateDirectories:YES
                                                        attributes:nil
                                                             error:&error];
                if (!created) {
                    MDZELog("Erreur création dossier: %@", error);
                    ret=false;
                    return ret;
                }
            }
            
            //[fileManager copyItemAtPath:fromPath toPath:toPath error:&error];
            NSArray *contents = [fileManager contentsOfDirectoryAtPath:fromPath error:&error];
            if (error) {
                MDZELog("Error saving to library: %@", error.localizedDescription);
                ret=false;
            }
            for (NSString *item in contents) {
                NSString *sourcePath = [fromPath stringByAppendingPathComponent:item];
                NSString *destPath = [toPath stringByAppendingPathComponent:item];
                [fileManager copyItemAtPath:sourcePath toPath:destPath error:&error];
                if (error) {
                    MDZELog("Error saving to library: %@", error.localizedDescription);
                    ret=false;
                }
            }
        } else ret=false;
    } else ret=false;
    return ret;
}


@end

NS_ASSUME_NONNULL_END
