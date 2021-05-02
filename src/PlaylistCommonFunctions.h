//
//  PlaylistCommonFunctions_h
//  modizer
//
//  Created by Yohann Magnien on 01/05/2021.
//

#ifndef PlaylistCommonFunctions_h
#define PlaylistCommonFunctions_h

#include <string.h>
typedef struct {
    char *pl_name;
    int pl_id;
    int pl_size;
} t_playlist_DB;

-(int) loadPlayListsListFromDB:(t_playlist_DB**)plList {
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


- (void) addToPlaylistSelView:(NSString*)fullPath label:(NSString*)label showNowListening:(bool)showNL{
    t_playlist_DB *plList;
    __block bool selDone=false;
    int plListsize=[self loadPlayListsListFromDB:&plList];
    
    
    UIAlertController *msgAlert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Add to a playlist",@"")
                                   message:[NSString stringWithFormat:NSLocalizedString(@"Choose a playlist",@"")]
                                   preferredStyle:UIAlertControllerStyleActionSheet];

    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
       handler:^(UIAlertAction * action) {
        if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
        
        selDone=true;
        }];
    [msgAlert addAction:cancelAction];

    if (showNL) {
#ifdef HAS_DETAILVIEW_CONT
        UIAlertAction* nowplayingAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Now playing",@"") style:UIAlertActionStyleDefault
           handler:^(UIAlertAction * action) {
                //NSLog(@"add to Now playing");
                if ([detailViewController add_to_playlist:fullPath fileName:label forcenoplay:1]) {
                    if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                }
                if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
            
                selDone=true;
                
            }];
        [msgAlert addAction:nowplayingAction];
#endif
    }
    
    for (int i=0;i<plListsize;i++) {
        UIAlertAction* userplaylistAction = [UIAlertAction actionWithTitle:[NSString stringWithFormat:@"%s",plList[i].pl_name] style:UIAlertActionStyleDefault
           handler:^(UIAlertAction * action) {
                [self addToPlaylistDB:[NSString stringWithFormat:@"%d",plList[i].pl_id]  label:label fullPath:fullPath];
            
                if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
                selDone=true;
                
            }];
        [msgAlert addAction:userplaylistAction];
    }
    
    [self presentViewController:msgAlert animated:YES completion:nil];
    while (!selDone) {
        [self flushMainLoop];
    }
 
}

- (void) addMultipleToPlaylistSelView:(NSArray*)arrayPath label:(NSArray*)arrayLabel showNowListening:(bool)showNL{
    t_playlist_DB *plList;
    __block bool selDone=false;
    int plListsize=[self loadPlayListsListFromDB:&plList];
    
    
    UIAlertController *msgAlert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Add to a playlist",@"")
                                   message:[NSString stringWithFormat:NSLocalizedString(@"Choose a playlist",@"")]
                                   preferredStyle:UIAlertControllerStyleActionSheet];

    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
       handler:^(UIAlertAction * action) {
        if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
        
        selDone=true;
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
            
                selDone=true;
                
            }];
        [msgAlert addAction:nowplayingAction];
#endif
    }
    
    for (int i=0;i<plListsize;i++) {
        UIAlertAction* userplaylistAction = [UIAlertAction actionWithTitle:[NSString stringWithFormat:@"%s",plList[i].pl_name] style:UIAlertActionStyleDefault
           handler:^(UIAlertAction * action) {
                [self addMultipleToPlaylistDB:[NSString stringWithFormat:@"%d",plList[i].pl_id]  labels:arrayLabel fullPaths:arrayPath];
            
                if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
                selDone=true;
                
            }];
        [msgAlert addAction:userplaylistAction];
    }
    
    [self presentViewController:msgAlert animated:YES completion:nil];
    while (!selDone) {
        [self flushMainLoop];
    }
 
}


#endif /* PlaylistCommonFunctions_h */
