//
//  PlaylistCommonFunctions_h
//  modizer
//
//  Created by Yohann Magnien on 01/05/2021.
//

#ifndef PlaylistCommonFunctions_h
#define PlaylistCommonFunctions_h

#include "AlertsCommonFunctions.h"

-(int) loadPlayListsListFromDB:(t_playlist_DB**)plList {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int pl_number=0;
    
    *plList=NULL;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT COUNT(*) FROM playlists");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                pl_number=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
        if (pl_number) {
            int pl_index=0;
            (*plList)=(t_playlist_DB*)calloc(1,sizeof(t_playlist_DB)*pl_number);
            
            sprintf(sqlStatement,"SELECT id,name,num_files FROM playlists ORDER BY name");
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    (*plList)[pl_index].pl_id=sqlite3_column_int(stmt, 0);
                    (*plList)[pl_index].pl_name=strdup((char*)sqlite3_column_text(stmt, 1));
                    (*plList)[pl_index].pl_size=sqlite3_column_int(stmt, 2);
                    pl_index++;
                }
                sqlite3_finalize(stmt);
            } else NSLog(@"ErrSQL : %d",err);
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    return pl_number;
}

-(int) getFavoritesCountFromDB {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int pl_number=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT COUNT(*) FROM user_stats WHERE rating=5");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                pl_number=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    return pl_number;
}

-(int) getMostPlayedCountFromDB {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int pl_number=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT COUNT(*) FROM user_stats WHERE play_count>0");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                pl_number=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    return pl_number;
}

-(int) loadMostPlayedList:(NSMutableArray*)labels fullpaths:(NSMutableArray*)fullpaths {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int pl_entries=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
            sprintf(sqlStatement,"SELECT name,fullpath FROM user_stats WHERE play_count>0 ORDER BY play_count DESC,name");
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    [labels addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)]];
                    [fullpaths addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)]];
                    pl_entries++;
                }
                sqlite3_finalize(stmt);
            } else NSLog(@"ErrSQL : %d",err);
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return pl_entries;
}

-(int) loadFavoritesList:(NSMutableArray*)labels fullpaths:(NSMutableArray*)fullpaths {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int pl_entries=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
            sprintf(sqlStatement,"SELECT name,fullpath FROM user_stats WHERE rating=5 ORDER BY rating DESC,name");
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    [labels addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)]];
                    [fullpaths addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)]];
                    pl_entries++;
                }
                sqlite3_finalize(stmt);
            } else NSLog(@"ErrSQL : %d",err);
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return pl_entries;
}

-(int) loadUserList:(int)pl_id  labels:(NSMutableArray*)labels fullpaths:(NSMutableArray*)fullpaths {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int pl_entries=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
                    
            sprintf(sqlStatement,"SELECT name,fullpath FROM playlists_entries WHERE id_playlist=%d",pl_id);
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    [labels addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)]];
                    [fullpaths addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)]];
                    pl_entries++;
                }
                sqlite3_finalize(stmt);
            } else NSLog(@"ErrSQL : %d",err);
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return pl_entries;
}


-(bool) addToPlaylistDB:(NSString*)id_playlist label:(NSString *)label fullPath:(NSString *)fullPath {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    bool result;
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
                [id_playlist UTF8String],[label UTF8String],[fullPath UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
            result=TRUE;
        } else {
            result=FALSE;
            NSLog(@"ErrSQL : %d",err);
        }
        if (result) {
            sprintf(sqlStatement,"UPDATE playlists SET num_files=\
                    (SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
                    WHERE id=%s",
                    [id_playlist UTF8String],[id_playlist UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
                result=TRUE;
            } else {
                result=FALSE;
                NSLog(@"ErrSQL : %d",err);
            }
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return result;
}

-(bool) addMultipleToPlaylistDB:(NSString*)id_playlist labels:(NSArray *)labels fullPaths:(NSArray *)fullPaths {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    bool result=TRUE;
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        for (int i=0;i<[labels count];i++) {
            sprintf(sqlStatement,"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
                    [id_playlist UTF8String],[[labels objectAtIndex:i] UTF8String],[[fullPaths objectAtIndex:i] UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
            } else {
                result=FALSE;
                NSLog(@"ErrSQL : %d",err);
                break;
            }
        }
        
        if (result) {
            sprintf(sqlStatement,"UPDATE playlists SET num_files=\
                    (SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
                    WHERE id=%s",
                    [id_playlist UTF8String],[id_playlist UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
                result=TRUE;
            } else {
                result=FALSE;
                NSLog(@"ErrSQL : %d",err);
            }
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return result;
}


/*- (void)createNewPlaylistWithEntries:(NSArray*)fullPaths labels:(NSArrays*)labels {
    UIAlertController *alertC = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Enter playlist name",@"")
                                        message:nil
                                        preferredStyle:UIAlertControllerStyleAlert];
    __weak UIAlertController *weakAlert = alertC;
    [alertC addTextFieldWithConfigurationHandler:^(UITextField *textField) {
        textField.placeholder = @"";
    }];
    
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
        [self freePlaylist];

    }];
    [alertC addAction:cancelAction];
    
    UIAlertAction *saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Save",@"") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        UITextField *plName = weakAlert.textFields.firstObject;
        if (![plName.text isEqualToString:@""]) {
            
            playlist->playlist_id=nil;
            playlist->playlist_name=nil;
            playlist->playlist_name=[[NSString alloc] initWithString:plName.text];
            playlist->playlist_id=[self minitNewPlaylistDB:playlist->playlist_name];
 .........
 
                        
        }
        else{
            [self presentViewController:weakAlert animated:YES completion:nil];
        }
    }];
    [alertC addAction:saveAction];
    
    [self showAlert:alertC];
}
*/


- (void) addToPlaylistSelView:(NSString*)fullPath label:(NSString*)label showNowListening:(bool)showNL{
    __block t_playlist_DB *plList;
    int plListsize=[self loadPlayListsListFromDB:&plList];
    
    UIAlertController *msgAlert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Add to a playlist",@"")
                                   message:[NSString stringWithFormat:NSLocalizedString(@"Choose a playlist",@"")]
                                   preferredStyle:UIAlertControllerStyleActionSheet];

    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
       handler:^(UIAlertAction * action) {
            if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
            //free playlists list
            for (int i=0;i<plListsize;i++) {
                mdz_safe_free(plList[i].pl_name);
            }
            mdz_safe_free(plList);
        }];
    [msgAlert addAction:cancelAction];
    
/*    UIAlertAction* newPlAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"New playlist",@"") style:UIAlertActionStyleDefault
       handler:^(UIAlertAction * action) {
            if ([detailViewController add_to_playlist:fullPath fileName:label forcenoplay:1]) {
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
            }
            if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
            //free playlists list
            for (int i=0;i<plListsize;i++) {
                mdz_safe_free(plList[i].pl_name);
            }
            mdz_safe_free(plList);
        }];
    [msgAlert addAction:newPlAction];*/

    if (showNL) {
#ifdef HAS_DETAILVIEW_CONT
        UIAlertAction* nowplayingAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Now playing",@"") style:UIAlertActionStyleDefault
           handler:^(UIAlertAction * action) {
                //NSLog(@"add to Now playing");
                if ([detailViewController add_to_playlist:fullPath fileName:label forcenoplay:1]) {
                    if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                }
                if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
                //free playlists list
                for (int i=0;i<plListsize;i++) {
                    mdz_safe_free(plList[i].pl_name);
                }
                mdz_safe_free(plList);
            }];
        [msgAlert addAction:nowplayingAction];
#endif
    }
    
    for (int i=0;i<plListsize;i++) {
        UIAlertAction* userplaylistAction = [UIAlertAction actionWithTitle:[NSString stringWithFormat:@"%s",plList[i].pl_name] style:UIAlertActionStyleDefault
           handler:^(UIAlertAction * action) {
                [self addToPlaylistDB:[NSString stringWithFormat:@"%d",plList[i].pl_id]  label:label fullPath:fullPath];
                if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
                //free playlists list
                for (int i=0;i<plListsize;i++) {
                    mdz_safe_free(plList[i].pl_name);
                }
                mdz_safe_free(plList);
            }];
        [msgAlert addAction:userplaylistAction];
    }
    [self showAlert:msgAlert];
}

- (void) addMultipleToPlaylistSelView:(NSArray*)arrayPath label:(NSArray*)arrayLabel showNowListening:(bool)showNL{
    __block t_playlist_DB *plList;
    plList=NULL;
    int plListsize=[self loadPlayListsListFromDB:&plList];
    
    
    UIAlertController *msgAlert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Add to a playlist",@"")
                                   message:[NSString stringWithFormat:NSLocalizedString(@"Choose a playlist",@"")]
                                   preferredStyle:UIAlertControllerStyleActionSheet];

    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
       handler:^(UIAlertAction * action) {
            if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
            //free playlists list
            for (int i=0;i<plListsize;i++) {
                mdz_safe_free(plList[i].pl_name);
            }
            mdz_safe_free(plList);
        }];
    [msgAlert addAction:cancelAction];

    if (showNL) {
#ifdef HAS_DETAILVIEW_CONT
        UIAlertAction* nowplayingAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Now playing",@"") style:UIAlertActionStyleDefault
           handler:^(UIAlertAction * action) {
                //NSLog(@"add to Now playing");
                if ([detailViewController add_to_playlist:arrayPath fileNames:arrayLabel forcenoplay:1]) {
                    if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                }
                if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
                //free playlists list
                for (int i=0;i<plListsize;i++) {
                    mdz_safe_free(plList[i].pl_name);
                }
                mdz_safe_free(plList);
            }];
        [msgAlert addAction:nowplayingAction];
#endif
    }
    
    for (int i=0;i<plListsize;i++) {
        UIAlertAction* userplaylistAction = [UIAlertAction actionWithTitle:[NSString stringWithFormat:@"%s",plList[i].pl_name] style:UIAlertActionStyleDefault
           handler:^(UIAlertAction * action) {
                [self addMultipleToPlaylistDB:[NSString stringWithFormat:@"%d",plList[i].pl_id]  labels:arrayLabel fullPaths:arrayPath];
                if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
                //free playlists list
                for (int i=0;i<plListsize;i++) {
                    mdz_safe_free(plList[i].pl_name);
                }
                mdz_safe_free(plList);
            }];
        [msgAlert addAction:userplaylistAction];
    }

    [self showAlert:msgAlert];
}

static NSArray * imp_RandomizeUsingMutableCopy(NSArray * arr) {
    if (1 >= arr.count) {
        return arr.copy;
    }
    NSMutableArray * cp = arr.mutableCopy;
    u_int32_t i = (u_int32_t)cp.count;
    while (i > 1) {
        --i;
        const u_int32_t j = arc4random_uniform(i);
        [cp exchangeObjectAtIndex:i withObjectAtIndex:j];
    }
    // you may not favor creating the concrete copy
    return cp.copy;
}

-(int) loadLocalFilesRandomPL:(NSMutableArray*)labels fullpaths:(NSMutableArray*)fullpaths {
    int pl_entries=0;
    NSString *file,*cpath;
    NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
    NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
    NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
    NSArray *filetype_extATARISOUND=[SUPPORTED_FILETYPE_ATARISOUND componentsSeparatedByString:@","];
    NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
    NSArray *filetype_extPT3=[SUPPORTED_FILETYPE_PT3 componentsSeparatedByString:@","];
    NSArray *filetype_extNSFPLAY=[SUPPORTED_FILETYPE_NSFPLAY componentsSeparatedByString:@","];
    NSArray *filetype_extPIXEL=[SUPPORTED_FILETYPE_PIXEL componentsSeparatedByString:@","];
    NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
    NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
    NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","];
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_ext2SF=[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","];
    NSArray *filetype_extV2M=[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSArray *filetype_extHC=[SUPPORTED_FILETYPE_HC componentsSeparatedByString:@","];
    NSArray *filetype_extEUP=[SUPPORTED_FILETYPE_EUP componentsSeparatedByString:@","];
    NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extS98=[SUPPORTED_FILETYPE_S98 componentsSeparatedByString:@","];
    NSArray *filetype_extKSS=[SUPPORTED_FILETYPE_KSS componentsSeparatedByString:@","];
    NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
    NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
    NSMutableArray *filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extMDX count]+[filetype_extPMD count]+[filetype_extSID count]+[filetype_extSTSOUND count]+[filetype_extATARISOUND count]+
                                  [filetype_extSC68 count]+[filetype_extPT3 count]+[filetype_extNSFPLAY count]+[filetype_extPIXEL count]+[filetype_extARCHIVE count]+[filetype_extUADE count]+[filetype_extMODPLUG count]+[filetype_extXMP count]+
                                  [filetype_extGME count]+[filetype_extADPLUG count]+[filetype_ext2SF count]+[filetype_extV2M count]+[filetype_extVGMSTREAM count]+
                                  [filetype_extHC count]+[filetype_extEUP count]+[filetype_extHVL count]+[filetype_extS98 count]+[filetype_extKSS count]+[filetype_extGSF count]+
                                  [filetype_extASAP count]+[filetype_extWMIDI count]+[filetype_extVGM count]];
    
    [filetype_ext addObjectsFromArray:filetype_extMDX];
    [filetype_ext addObjectsFromArray:filetype_extPMD];
    [filetype_ext addObjectsFromArray:filetype_extSID];
    [filetype_ext addObjectsFromArray:filetype_extSTSOUND];
    [filetype_ext addObjectsFromArray:filetype_extATARISOUND];
    [filetype_ext addObjectsFromArray:filetype_extSC68];
    [filetype_ext addObjectsFromArray:filetype_extPT3];
    [filetype_ext addObjectsFromArray:filetype_extNSFPLAY];
    [filetype_ext addObjectsFromArray:filetype_extPIXEL];
    [filetype_ext addObjectsFromArray:filetype_extARCHIVE];
    [filetype_ext addObjectsFromArray:filetype_extUADE];
    [filetype_ext addObjectsFromArray:filetype_extMODPLUG];
    [filetype_ext addObjectsFromArray:filetype_extXMP];
    [filetype_ext addObjectsFromArray:filetype_extGME];
    [filetype_ext addObjectsFromArray:filetype_extADPLUG];
    [filetype_ext addObjectsFromArray:filetype_ext2SF];
    [filetype_ext addObjectsFromArray:filetype_extV2M];
    [filetype_ext addObjectsFromArray:filetype_extVGMSTREAM];
    [filetype_ext addObjectsFromArray:filetype_extHC];
    [filetype_ext addObjectsFromArray:filetype_extEUP];
    [filetype_ext addObjectsFromArray:filetype_extHVL];
    [filetype_ext addObjectsFromArray:filetype_extS98];
    [filetype_ext addObjectsFromArray:filetype_extKSS];
    [filetype_ext addObjectsFromArray:filetype_extGSF];
    [filetype_ext addObjectsFromArray:filetype_extASAP];
    [filetype_ext addObjectsFromArray:filetype_extWMIDI];
    [filetype_ext addObjectsFromArray:filetype_extVGM];
    
    // First check count for each section
    cpath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
    NSError *error;
    NSArray *dirContent;//
    BOOL isDir;
    
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
        
    dirContent=[mFileMngr subpathsOfDirectoryAtPath:cpath error:&error];
    
    NSArray *sortedDirContent = imp_RandomizeUsingMutableCopy(dirContent);
    
    for (int i=0;i<[sortedDirContent count];i++) {
        NSString *file=[sortedDirContent objectAtIndex:i];
        //check if dir
        [mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
        if (isDir) {
        } else {
            NSString *extension;// = [[file pathExtension] uppercaseString];
            NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
            NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
            extension = (NSString *)[temparray_filepath lastObject];
            //[temparray_filepath removeLastObject];
            file_no_ext=[temparray_filepath firstObject];
                                                
            int found=0;
            
            if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
            else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
            
            if (found)  {
                [labels addObject:[NSString stringWithString:[file lastPathComponent]]];
                [fullpaths addObject:[NSString stringWithFormat:@"Documents/%@",file]];
                pl_entries++;
                if (pl_entries>=MAX_CARPLAY_RANDOM_PL_SIZE) break;
            }
        }
    }
    
    return pl_entries;
}

#endif /* PlaylistCommonFunctions_h */
