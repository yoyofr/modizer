/*
 *  DBHelper.mm
 *  modizer
 *
 *  Created by Yohann Magnien on 23/08/10.
 *  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
 *
 */

#include "DBHelper.h"

#import "ModizFileHelper.h"

#include "ModizerConstants.h"
#include "sqlite3.h"
#include <pthread.h>

extern pthread_mutex_t db_mutex;

NSMutableArray *DBHelper::getMissingPartsNameFromFilePath(NSString *fullPath,NSString *ext) {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    NSMutableArray *result=[[NSMutableArray alloc] init];
    
    if (fullPath==nil) return nil;
    NSArray *strComponents=[fullPath componentsSeparatedByString:@"/"];
    if ([strComponents count]<5) return nil;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];//,sqltmp[512];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        const char *sqlAuthor,*sqlFiletype,*sqlAlbum,*sqlFilename;
        sqlAuthor=[[strComponents objectAtIndex:2] UTF8String];
        sqlFiletype=[[strComponents objectAtIndex:3] UTF8String];
        if ([strComponents count]>5) {
            sqlAlbum=[[strComponents objectAtIndex:4] UTF8String];
            sqlFilename=[[strComponents lastObject] UTF8String];
        } else {
            sqlAlbum=NULL;
            sqlFilename=[[strComponents lastObject] UTF8String];
        }
        
        if (sqlAlbum) {
            snprintf(sqlStatement,1024,""
                     "SELECT f.fullpath,a.author||'/'||t.filetype||'/'||l.album||'/'||f.filename "
                     "FROM mod_author a,mod_type t,mod_album l, mod_file f "
                     "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album=l.id "
                     "AND a.author like \"%s\" AND t.filetype like \"%s\" AND l.album like \"%s\" AND f.filename like \"%%%s\" "
                     "",sqlAuthor,sqlFiletype,sqlAlbum,[ext UTF8String]);
        } else {
            snprintf(sqlStatement,1024,""
                     "SELECT f.fullpath,a.author||'/'||t.filetype||'/'||f.filename "
                     "FROM mod_author a,mod_type t, mod_file f "
                     "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album IS NULL "
                     "AND a.author like \"%s\" AND t.filetype like \"%s\" AND f.filename like \"%%%s\" "
                     "",sqlAuthor,sqlFiletype,[ext UTF8String]);
        }
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                NSString *localPath=[NSString stringWithFormat:@"%@/Documents/%@",[ModizFileHelper getAppHomeDirectory],[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)]];
                
                FILE *f=fopen([localPath UTF8String],"rb");
                if (!f) {
                    [result addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)]];
                    [result addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)]];
                }
                else fclose(f);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return result;
}

NSMutableArray *DBHelper::getMissingPartsNameFromRemotePath(NSString *fullPath,NSString *ext) {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    NSMutableArray *result=[[NSMutableArray alloc] init];
    
    if (fullPath==nil) return nil;
    NSArray *strComponents=[fullPath componentsSeparatedByString:@"/"];
    if ([strComponents count]<3) return nil;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];//,sqltmp[512];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        const char *sqlAuthor,*sqlFiletype,*sqlAlbum,*sqlFilename;
        sqlAuthor=[[strComponents objectAtIndex:1] UTF8String];
        sqlFiletype=[[strComponents objectAtIndex:0] UTF8String];
        if ([strComponents count]>3) {
            sqlAlbum=[[strComponents objectAtIndex:2] UTF8String];
            sqlFilename=[[strComponents lastObject] UTF8String];
        } else {
            sqlAlbum=NULL;
            sqlFilename=[[strComponents lastObject] UTF8String];
        }
        
        if (sqlAlbum) {
            snprintf(sqlStatement,1024,""
                     "SELECT f.fullpath,a.author||'/'||t.filetype||'/'||l.album||'/'||f.filename "
                     "FROM mod_author a,mod_type t,mod_album l, mod_file f "
                     "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album=l.id "
                     "AND a.author like \"%s\" AND t.filetype like \"%s\" AND l.album like \"%s\" AND f.filename like \"%%%s\" "
                     "",sqlAuthor,sqlFiletype,sqlAlbum,[ext UTF8String]);
        } else {
            snprintf(sqlStatement,1024,""
                     "SELECT f.fullpath,a.author||'/'||t.filetype||'/'||f.filename "
                     "FROM mod_author a,mod_type t, mod_file f "
                     "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album IS NULL "
                     "AND a.author like \"%s\" AND t.filetype like \"%s\" AND f.filename like \"%%%s\" "
                     "",sqlAuthor,sqlFiletype,[ext UTF8String]);
        }
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                NSString *localPath=[NSString stringWithFormat:@"%@/Documents/%@",[ModizFileHelper getAppHomeDirectory],[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)]];
                
                FILE *f=fopen([localPath UTF8String],"rb");
                if (!f) {
                    [result addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)]];
                    [result addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)]];
                }
                else fclose(f);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return result;
}


NSString *DBHelper::getFullPathFromLocalPath(NSString *localPath) {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    NSString *result=nil;
    if (localPath==nil) return nil;
    
    NSArray *strComponents=[localPath componentsSeparatedByString:@"/"];
    if ([strComponents count]<3) return nil;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];//,sqltmp[512];
        int adjusted=0;
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //snprintf(sqltmp,512,"%s",[localPath UTF8String]);
        //		printf("%s\n",sqltmp);

        const char *sqlAuthor,*sqlFiletype,*sqlAlbum,*sqlFilename;
        sqlAuthor=[[strComponents objectAtIndex:0] UTF8String];
        sqlFiletype=[[strComponents objectAtIndex:1] UTF8String];
        if ([strComponents count]>3) {
            sqlAlbum=[[strComponents objectAtIndex:2] UTF8String];
            sqlFilename=[[strComponents lastObject] UTF8String];
        } else {
            sqlAlbum=NULL;
            sqlFilename=[[strComponents lastObject] UTF8String];
        }
        
        if (adjusted) {
            if (sqlAlbum) {
                snprintf(sqlStatement,1024,""
                         "SELECT f.fullpath "
                         "FROM mod_author a,mod_type t,mod_album l, mod_file f "
                         "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album=l.id "
                         "AND a.author like \"%s\" AND t.filetype like \"%s\" AND l.album like \"%s\" AND f.filename like \"%s\" "
                         "",sqlAuthor,sqlFiletype,sqlAlbum,sqlFilename);
            } else {
                snprintf(sqlStatement,1024,""
                         "SELECT f.fullpath "
                         "FROM mod_author a,mod_type t, mod_file f "
                         "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album IS NULL "
                         "AND a.author like \"%s\" AND t.filetype like \"%s\" AND f.filename like \"%s\" "
                         "",sqlAuthor,sqlFiletype,sqlFilename);
            }
        }
        else {
            if (sqlAlbum) {
                snprintf(sqlStatement,1024,""
                         "SELECT f.fullpath "
                         "FROM mod_author a,mod_type t,mod_album l, mod_file f "
                         "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album=l.id "
                         "AND a.author = \"%s\" AND t.filetype = \"%s\" AND l.album = \"%s\" AND f.filename = \"%s\" "
                         "",sqlAuthor,sqlFiletype,sqlAlbum,sqlFilename);
            } else {
                snprintf(sqlStatement,1024,""
                         "SELECT f.fullpath "
                         "FROM mod_author a,mod_type t, mod_file f "
                         "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album IS NULL "
                         "AND a.author = \"%s\" AND t.filetype = \"%s\" AND f.filename = \"%s\" "
                         "",sqlAuthor,sqlFiletype,sqlFilename);
            }
        }
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                result=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return result;
}

NSString *DBHelper::getLocalPathFromFullPath(NSString *fullPath) {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    NSString *result=nil;
    
    if (fullPath==nil) return nil;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024],sqltmp[512];
        int adjusted=0;
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqltmp,512,"%s",[fullPath cStringUsingEncoding:NSNonLossyASCIIStringEncoding]);
        
        char *tmp_str;
        tmp_str=strstr(sqltmp,"\\u");
        while (tmp_str) {
            if (tmp_str>sqltmp) {
                tmp_str--;
                *tmp_str='%';
                memmove(tmp_str+1,tmp_str+7,strlen(tmp_str)-6);
                tmp_str=strstr(tmp_str,"\\u");
                adjusted=1;
            }
        }
        if (adjusted) snprintf(sqlStatement,1024,""
                               "SELECT a.author||'/'||t.filetype||'/'||l.album||'/'||f.filename "
                               "FROM mod_author a,mod_type t,mod_album l, mod_file f "
                               "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album=l.id AND f.fullpath like \"%s\" "
                               "UNION "
                               "SELECT a.author||'/'||t.filetype||'/'||f.filename "
                               "FROM mod_author a,mod_type t, mod_file f "
                               "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album IS NULL AND f.fullpath like \"%s\" "
                               "",sqltmp,sqltmp);
        else snprintf(sqlStatement,1024,""
                      "SELECT a.author||'/'||t.filetype||'/'||l.album||'/'||f.filename "
                      "FROM mod_author a,mod_type t,mod_album l, mod_file f "
                      "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album=l.id AND f.fullpath = \"%s\" "
                      "UNION "
                      "SELECT a.author||'/'||t.filetype||'/'||f.filename "
                      "FROM mod_author a,mod_type t, mod_file f "
                      "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album IS NULL AND f.fullpath = \"%s\" "
                      "",sqltmp,sqltmp);
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                result=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return result;
}

NSString *DBHelper::getCleanStr(NSString *str) {
    return [str stringByReplacingOccurrencesOfString:@"\"" withString:@"\"\""];
}

int DBHelper::getFileStatsDBmod(NSString *fullpath,short int *playcount,signed char *rating,signed char *avg_rating,int *song_length,char *channels_nb,int *songs) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    int ret=0;
    
    if (fullpath==nil) return ret;
    
    if (playcount) *playcount=0;
    if (rating) *rating=0;
    if (avg_rating) *avg_rating=0;
    if (song_length) *song_length=0;
    if (channels_nb) *channels_nb=0;
    if (songs) *songs=0;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT play_count,rating,avg_rating,length,channels,songs FROM user_stats WHERE fullpath=\"%s\"",[DBHelper::getCleanStr(fullpath) UTF8String]);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret++;
                if (playcount) *playcount=(short int)sqlite3_column_int(stmt, 0);
                if (rating) {
                    *rating=(signed char)sqlite3_column_int(stmt, 1);
                    if (*rating<0) *rating=0;
                    if (*rating>5) *rating=5;
                }
                if (avg_rating) {
                    if (sqlite3_column_type(stmt,2)!=SQLITE_NULL) *avg_rating=(signed char)sqlite3_column_int(stmt, 2);
                    if (*avg_rating<0) *avg_rating=0;
                    if (*avg_rating>5) *avg_rating=5;
                }
                if (song_length) *song_length=(int)sqlite3_column_int(stmt, 3);
                if (channels_nb) *channels_nb=(char)sqlite3_column_int(stmt, 4);
                if (songs) *songs=(int)sqlite3_column_int(stmt, 5);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret;
}

int DBHelper::deleteStatsFileDB(NSString *fullpath) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err,ret;
    
    if (fullpath==nil) return -1;
    fullpath=[ModizFileHelper getFilePathFromDocuments:fullpath];
    
    pthread_mutex_lock(&db_mutex);
    ret=1;
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //Remove stats
        snprintf(sqlStatement,1024,"DELETE FROM user_stats WHERE fullpath = \"%s\"",[DBHelper::getCleanStr(fullpath) UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else {ret=0;MDZELog("ErrSQL : %d",err);}
        
        //Update playlists referencing the deleted file
        snprintf(sqlStatement,1024,"SELECT id_playlist FROM playlists_entries WHERE fullpath =\"%s\"",[DBHelper::getCleanStr(fullpath) UTF8String]);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int plid;
                plid=(int)sqlite3_column_int(stmt, 0);
        
                //Remove stats
                snprintf(sqlStatement,1024,"DELETE FROM playlists_entries WHERE id_playlist=%d AND fullpath = \"%s\"",plid,[DBHelper::getCleanStr(fullpath) UTF8String]);
                err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
                if (err==SQLITE_OK){
                } else {ret=0;MDZELog("ErrSQL : %d",err);}
                
                //Recompute nb of entries
                snprintf(sqlStatement,1024,"UPDATE playlists SET num_files=\
                        (SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%d)\
                        WHERE id=%d",
                         plid,plid);
                err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
                if (err==SQLITE_OK){
                } else {ret=0;MDZELog("ErrSQL : %d",err);}
                
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return ret;
}
int DBHelper::deleteStatsDirDB(NSString *fullpath) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err,ret;
    
    if (fullpath==nil) return -1;
    fullpath=[ModizFileHelper getFilePathFromDocuments:fullpath];
    
    pthread_mutex_lock(&db_mutex);
    ret=1;
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"DELETE FROM user_stats WHERE fullpath LIKE \"%s/%%\"",[DBHelper::getCleanStr(fullpath) UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else {ret=0;MDZELog("ErrSQL : %d",err);}
        
        //Update playlists referencing the deleted file
        snprintf(sqlStatement,1024,"SELECT id_playlist FROM playlists_entries WHERE fullpath LIKE \"%s/%%\"",[DBHelper::getCleanStr(fullpath) UTF8String]);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int plid;
                plid=(int)sqlite3_column_int(stmt, 0);
        
                //Remove stats
                snprintf(sqlStatement,1024,"DELETE FROM playlists_entries WHERE id_playlist=%d AND fullpath LIKE \"%s/%%\"",plid,[DBHelper::getCleanStr(fullpath) UTF8String]);
                err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
                if (err==SQLITE_OK){
                } else {ret=0;MDZELog("ErrSQL : %d",err);}
                
                //Recompute nb of entries
                snprintf(sqlStatement,1024,"UPDATE playlists SET num_files=\
                        (SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%d)\
                        WHERE id=%d",
                         plid,plid);
                err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
                if (err==SQLITE_OK){
                } else {ret=0;MDZELog("ErrSQL : %d",err);}
                
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return ret;
}

int DBHelper::getNbFormatEntries() {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    int ret_int=0;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT count(1) FROM mod_type");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret_int=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret_int;
}
int DBHelper::getNbAuthorEntries() {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    int ret_int=0;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT count(1) FROM mod_author");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret_int=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret_int;
}
int DBHelper::getNbHVSCFilesEntries() {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    int ret_int=0;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT count(1) FROM hvsc_file");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret_int=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret_int;
}
int DBHelper::getNbCGSCFilesEntries() {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    int ret_int=0;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT count(1) FROM cgsc_file");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret_int=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret_int;
}

int DBHelper::getNbASMAFilesEntries() {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    int ret_int=0;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT count(1) FROM asma_file");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret_int=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret_int;
}
int DBHelper::getNbMODLANDFilesEntries() {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    int ret_int=0;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT count(1) FROM mod_file");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret_int=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret_int;
}

void DBHelper::updateFileStatsAvgRatingDBmod(NSString *fullpath) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    int avg_rating;
    int global_rating;
    int entries_nb;
    int playcount,sng_length,channels,songs;
    bool isMultisongs=false;
    bool isArchive=false;
    NSString *fullpathCleaned;
    NSString *fname;
    if (fullpath==nil) return;
    
    if ([fullpath rangeOfString:@"?"].location!=NSNotFound) isMultisongs=true;
    if ([fullpath rangeOfString:@"@"].location!=NSNotFound) isArchive=true;
    
    if (isMultisongs) {
        //1st compute avg rating for multisong entry
        fullpathCleaned=[ModizFileHelper getFilePathNoSubSong:fullpath];
        
        avg_rating=0;
        global_rating=0;
        entries_nb=0;
        playcount=0;
        fname=NULL;
        
        pthread_mutex_lock(&db_mutex);
        
        //1st get all related entries (archive entries & subsongs)
        if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
            char sqlStatement[1024];
            sqlite3_stmt *stmt;
            
            err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
            if (err==SQLITE_OK){
            } else MDZELog("ErrSQL : %d",err);
            
            snprintf(sqlStatement,1024,"SELECT name,fullpath,play_count,rating,length,channels,songs FROM user_stats WHERE fullpath like \"%s%%\"",[DBHelper::getCleanStr(fullpathCleaned) UTF8String]);
            
            //printf("req: %s\n",sqlStatement);
            
            int fullpath_len=[fullpathCleaned length];
            
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    int tmp_rating;
                    char *tmp_fullpath;
                    tmp_fullpath=(char*)sqlite3_column_text(stmt,1);
                    
                    if (strlen(tmp_fullpath)>fullpath_len) {
                        
                        tmp_rating=(signed char)sqlite3_column_int(stmt, 3);
                        
                        //printf("entry #%d, name: %s, rating: %d\n",entries_nb,tmp_fullpath,tmp_rating);
                        
                        if (tmp_rating<0) tmp_rating=0;
                        if (tmp_rating>5) tmp_rating=5;
                        avg_rating+=tmp_rating;
                        //if (tmp_rating>0)
                        entries_nb++;
                    } else {
                        //entry for the global file
                        fname=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                        playcount=(int)sqlite3_column_int(stmt, 2);
                        global_rating=(int)sqlite3_column_int(stmt, 3);
                        sng_length=(int)sqlite3_column_int(stmt, 4);
                        channels=(int)sqlite3_column_int(stmt, 5);
                        songs=(int)sqlite3_column_int(stmt, 6);
                    }
                }
                if (entries_nb&&(songs>0)) {
                    /*if (avg_rating>0) {
                     avg_rating=avg_rating/entries_nb;
                     if (avg_rating==0) avg_rating=1;
                     }*/
                    if (avg_rating>0) {
                        avg_rating=abs(avg_rating/songs);
                        if (!avg_rating) avg_rating=1;
                    }
                } else avg_rating=0;
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
            
            //2nd update rating based on average
            
            snprintf(sqlStatement,1024,"DELETE FROM user_stats WHERE fullpath=\"%s\"",[DBHelper::getCleanStr(fullpathCleaned) UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
            } else MDZELog("ErrSQL : %d",err);
            
            if (fname==NULL) fname=[fullpathCleaned lastPathComponent];
            
            snprintf(sqlStatement,1024,"INSERT INTO user_stats (name,fullpath,play_count,rating,avg_rating,length,channels,songs) SELECT \"%s\",\"%s\",%d,%d,%d,%d,%d,%d",[DBHelper::getCleanStr(fname) UTF8String],[DBHelper::getCleanStr(fullpathCleaned) UTF8String],playcount,global_rating,avg_rating,sng_length,channels,songs);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
            } else MDZELog("ErrSQL : %d",err);
            
        }
        sqlite3_close(db);
        
        pthread_mutex_unlock(&db_mutex);
    }
    
    if (isArchive) {
        fullpathCleaned=[ModizFileHelper getFullCleanFilePath:fullpath];
        
        avg_rating=0;
        global_rating=0;
        entries_nb=0;
        playcount=0;
        fname=NULL;
        
        pthread_mutex_lock(&db_mutex);
        
        //1st get all related entries (archive entries & subsongs)
        if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
            char sqlStatement[1024];
            sqlite3_stmt *stmt;
            
            err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
            if (err==SQLITE_OK){
            } else MDZELog("ErrSQL : %d",err);
            
            snprintf(sqlStatement,1024,"SELECT name,fullpath,play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath like \"%s%%\"",[DBHelper::getCleanStr(fullpathCleaned) UTF8String]);
            
            //printf("req: %s\n",sqlStatement);
            
            int fullpath_len=[fullpathCleaned length];
            
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    int tmp_rating;
                    char *tmp_fullpath;
                    tmp_fullpath=(char*)sqlite3_column_text(stmt,1);
                    
                    //filter subsong entries
                    if (strchr(tmp_fullpath,'?')==NULL) {
                        //check if not global file
                        if (strlen(tmp_fullpath)>fullpath_len) {
                            //got an entry, get rating
                            tmp_rating=(signed char)sqlite3_column_int(stmt, 3);
                            if (tmp_rating==0) {
                                //if 0, try avg_rating
                                if (sqlite3_column_type(stmt, 7)!=SQLITE_NULL) tmp_rating=(signed char)sqlite3_column_int(stmt, 7);
                            }
                            
                            if (tmp_rating<0) tmp_rating=0;
                            if (tmp_rating>5) tmp_rating=5;
                            avg_rating+=tmp_rating;
                            //if (tmp_rating>0)
                            entries_nb++;
                        } else {
                            //entry for the global file
                            fname=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                            playcount=(int)sqlite3_column_int(stmt, 2);
                            global_rating=(int)sqlite3_column_int(stmt, 3);
                            sng_length=(int)sqlite3_column_int(stmt, 4);
                            channels=(int)sqlite3_column_int(stmt, 5);
                            songs=(int)sqlite3_column_int(stmt, 6);
                        }
                    }
                }
                if (entries_nb&&(abs(songs)>0)) {
                    /*if (avg_rating>0) {
                     avg_rating=avg_rating/entries_nb;
                     if (avg_rating==0) avg_rating=1;
                     }*/
                    if (avg_rating>0) {
                        avg_rating=abs(avg_rating/songs);
                        if (!avg_rating) avg_rating=1;
                    }
                } else avg_rating=0;
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
            
            //2nd update rating based on average
            
            snprintf(sqlStatement,1024,"DELETE FROM user_stats WHERE fullpath=\"%s\"",[DBHelper::getCleanStr(fullpathCleaned) UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
            } else MDZELog("ErrSQL : %d",err);
            
            if (fname==NULL) fname=[fullpathCleaned lastPathComponent];
            
            snprintf(sqlStatement,1024,"INSERT INTO user_stats (name,fullpath,play_count,rating,avg_rating,length,channels,songs) SELECT \"%s\",\"%s\",%d,%d,%d,%d,%d,%d",[DBHelper::getCleanStr(fname) UTF8String],[DBHelper::getCleanStr(fullpathCleaned) UTF8String],playcount,global_rating,avg_rating,sng_length,channels,songs);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
            } else MDZELog("ErrSQL : %d",err);
            
        }
        sqlite3_close(db);
        
        pthread_mutex_unlock(&db_mutex);
    }
    
    
}

int DBHelper::updateFileStatsDBmod(NSString *name,NSString *fullpath,short int playcount,signed char rating,signed char avg_rating,int song_length,signed char channels_nb,int songs) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    int ret=0;
    
    if (name==NULL) return ret;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT name,play_count,length,channels,songs,rating,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[DBHelper::getCleanStr(fullpath) UTF8String]);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret++;
                if (name==NULL) name=[NSString stringWithUTF8String:(const char*)(sqlite3_column_text(stmt,0))];
                if (playcount==-1) playcount=(short int)sqlite3_column_int(stmt, 1);
                if (song_length==-1) song_length=(int)sqlite3_column_int(stmt, 2);
                if (channels_nb==-1) channels_nb=(char)sqlite3_column_int(stmt, 3);
                if (songs==-1) songs=(int)sqlite3_column_int(stmt, 4);
                if (rating==-1) rating=(int)sqlite3_column_int(stmt, 5);
                if (avg_rating==-1) avg_rating=(int)sqlite3_column_int(stmt, 6);
                
                break;
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"DELETE FROM user_stats WHERE fullpath=\"%s\"",[DBHelper::getCleanStr(fullpath) UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        if (playcount==-1) playcount=0;
        if (rating==-1) rating=0;
        if (avg_rating==-1) avg_rating=0;
        
        snprintf(sqlStatement,1024,"INSERT INTO user_stats (name,fullpath,play_count,rating,avg_rating,length,channels,songs) SELECT \"%s\",\"%s\",%d,%d,%d,%d,%d,%d",[DBHelper::getCleanStr(name) UTF8String],[DBHelper::getCleanStr(fullpath) UTF8String],playcount,rating,avg_rating,song_length,channels_nb,songs);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret;
}

int DBHelper::updateRatingDBmod(NSString *fullpath,signed char rating) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    int ret=0;
    
    if (fullpath==nil) return ret;
    if ([fullpath isEqualToString:@""]) return ret;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        short int playcount=0;
        int song_length=0;
        char channels_nb=0;
        char *name=NULL;
        int songs=0;
        char avg_rating=0;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT name,play_count,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[DBHelper::getCleanStr(fullpath) UTF8String]);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret++;
                name=(char*)strdup((const char*)(sqlite3_column_text(stmt,0)));
                playcount=(short int)sqlite3_column_int(stmt, 1);
                song_length=(int)sqlite3_column_int(stmt, 2);
                channels_nb=(char)sqlite3_column_int(stmt, 3);
                songs=(int)sqlite3_column_int(stmt, 4);
                avg_rating=(signed char)sqlite3_column_int(stmt, 5);
                
                break;
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,sizeof(sqlStatement),"DELETE FROM user_stats WHERE fullpath=\"%s\"",[DBHelper::getCleanStr(fullpath) UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        if (name==NULL) {
            name=(char*)strdup([[DBHelper::getCleanStr(fullpath) lastPathComponent] UTF8String]);
        } else {
            NSString *nameTmp=[NSString stringWithUTF8String:name];
            free(name);
            name=(char*)strdup([[DBHelper::getCleanStr(nameTmp) lastPathComponent] UTF8String]);
        }
        
        snprintf(sqlStatement,sizeof(sqlStatement),"INSERT INTO user_stats (name,fullpath,play_count,rating,avg_rating,length,channels,songs) SELECT \"%s\",\"%s\",%d,%d,%d,%d,%d,%d",name,[DBHelper::getCleanStr(fullpath) UTF8String],playcount,rating,avg_rating,song_length,channels_nb,songs);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        if (name) free(name);
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    
    return ret;
}

bool dbhelper_cancel;
char cleanDB_Status[1024];

int DBHelper::cleanDB() {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    BOOL success;
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        char sqlStatement2[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //---------------------------------------------------
        //Check if tables structure is up-to-date
        //---------------------------------------------------
        
        if (dbhelper_cancel) {
            printf("Cancelling\n");
            sqlite3_close(db);
            pthread_mutex_unlock(&db_mutex);
            fileManager=nil;
            return -1;
        }
        
        printf("checking structure\n");
        snprintf(cleanDB_Status,sizeof(cleanDB_Status),"checking structure");
        
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT sql FROM sqlite_schema WHERE name='user_stats'");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                if (strstr((const char*)sqlite3_column_text(stmt, 0),"avg_rating")==NULL) {
                    //---------------------------------------------------
                    //missing avg_rating column added in v3.6, create it
                    //---------------------------------------------------
                    snprintf(sqlStatement2,sizeof(sqlStatement2),"ALTER TABLE user_stats ADD COLUMN avg_rating integer");
                    err=sqlite3_exec(db, sqlStatement2, NULL, NULL, NULL);
                    if (err!=SQLITE_OK) {
                        MDZELog("Issue during add of avg_rating column in user_stats");
                    }
                }
                break;
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        int checked_entries=0;
        int cleaned_entries=0;
        printf("checking user_stats entries\n");
        snprintf(cleanDB_Status,sizeof(cleanDB_Status),"checking user_stats entries");
        //First check that user_stats entries still exist
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT fullpath FROM user_stats");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                if (dbhelper_cancel) {
                    printf("Cancelling\n");
                    break;
                }
                
                checked_entries++;
                
                NSString *fullpath=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                //clean up for archive/multisong entries
                fullpath=[ModizFileHelper getFullCleanFilePath:fullpath];
                success = [fileManager fileExistsAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:fullpath]];
                if (!success) {//file does not exist
                    cleaned_entries++;
                    
                    snprintf(sqlStatement2,sizeof(sqlStatement2),"DELETE FROM user_stats WHERE fullpath LIKE \"%s%%\"",[DBHelper::getCleanStr([NSString stringWithUTF8String:(const char *)sqlite3_column_text(stmt, 0)]) UTF8String] );
                    err=sqlite3_exec(db, sqlStatement2, NULL, NULL, NULL);
                    if (err!=SQLITE_OK) {
                        MDZELog("Issue during delete of user_stats, err:%d",err);
                        MDZELog("%s",sqlStatement2);
                    }
                }
                
                snprintf(cleanDB_Status,sizeof(cleanDB_Status),"checked: %d, removed: %d",checked_entries,cleaned_entries);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        printf("checked: %d, removed: %d\n",checked_entries,cleaned_entries);
        snprintf(cleanDB_Status,sizeof(cleanDB_Status),"checked: %d, removed: %d",checked_entries,cleaned_entries);
        
        if (dbhelper_cancel) {
            printf("Cancelling\n");
            sqlite3_close(db);
            pthread_mutex_unlock(&db_mutex);
            fileManager=nil;
            return -1;
        }
        
        checked_entries=0;
        cleaned_entries=0;
        printf("checking playlists entries\n");
        snprintf(cleanDB_Status,sizeof(cleanDB_Status),"checking playlists entries");
        //Second check that playlist entries still exist
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT fullpath,name FROM playlists_entries");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                if (dbhelper_cancel) {
                    printf("Cancelling\n");
                    break;
                }
                checked_entries++;
                 
                NSString *fullpath=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                //clean up for archive/multisong entries
                fullpath=[ModizFileHelper getFullCleanFilePath:fullpath];
                success = [fileManager fileExistsAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:fullpath]];
                if (!success) {//file does not exist
                    MDZILog("missing : %s",sqlite3_column_text(stmt, 0));
                    
                    cleaned_entries++;
                    
                    snprintf(sqlStatement2,sizeof(sqlStatement2),"DELETE FROM playlists_entries WHERE fullpath=\"%s\"",sqlite3_column_text(stmt, 0));
                    err=sqlite3_exec(db, sqlStatement2, NULL, NULL, NULL);
                    if (err!=SQLITE_OK) {
                        MDZELog("Issue during delete of playlists_entries");
                    }
                } else {
                    NSString *tmpns=[NSString stringWithUTF8String:(char*)sqlite3_column_text(stmt, 1)];
                    if ([tmpns containsString:@"/"]) {
                        snprintf(sqlStatement2,sizeof(sqlStatement2),"UPDATE playlists_entries SET name=\"%s\" WHERE fullpath=\"%s\"",
                                 [[tmpns lastPathComponent] UTF8String],
                                 sqlite3_column_text(stmt, 0));
                        err=sqlite3_exec(db, sqlStatement2, NULL, NULL, NULL);
                        if (err!=SQLITE_OK) {
                            MDZELog("Issue during delete of playlists_entries");
                        }
                    }
                }
                
                snprintf(cleanDB_Status,sizeof(cleanDB_Status),"checked: %d, removed: %d",checked_entries,cleaned_entries);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        printf("checked: %d, removed: %d\n",checked_entries,cleaned_entries);
        snprintf(cleanDB_Status,sizeof(cleanDB_Status),"checked: %d, removed: %d",checked_entries,cleaned_entries);
        
        printf("updating playlists size\n");
        snprintf(cleanDB_Status,sizeof(cleanDB_Status),"updating playlists size");
        //update playlist table / entries
        snprintf(sqlStatement,sizeof(sqlStatement),"UPDATE playlists SET num_files=\
                (SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist)");
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err!=SQLITE_OK){
            MDZELog("ErrSQL : %d",err);
        }
        
        //Clean v4.1- garbage
        //
        snprintf(cleanDB_Status,sizeof(cleanDB_Status),"removing wrong entries from user_stats / rsn files");
        snprintf(sqlStatement,sizeof(sqlStatement),"DELETE FROM user_stats WHERE fullpath LIKE '%%rsn%%?%%'");
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err!=SQLITE_OK){
            MDZELog("ErrSQL : %d",err);
        }
        
        printf("compressing DB\n");
        snprintf(cleanDB_Status,sizeof(cleanDB_Status),"compressing DB");
        //No defrag DB
        snprintf(sqlStatement2,sizeof(sqlStatement2),"VACUUM");
        err=sqlite3_exec(db, sqlStatement2, NULL, NULL, NULL);
        if (err!=SQLITE_OK) {
            MDZELog("Issue during VACUUM, err: %d",err);
        }
    } else {
        printf("Cannot open DB\n");
    }
    sqlite3_close(db);
    
    
    
    pthread_mutex_unlock(&db_mutex);
    fileManager=nil;
    
    return 0;
}

int DBHelper::getRating(NSString *filePath,int arcidx,int subidx) {
    signed char rating;
    NSString *fpath;
    fpath=[ModizFileHelper getFullCleanFilePath:[NSString stringWithString:filePath]];
    if (arcidx>=0) {
        /////////////////////////////////////////////////////////////////////////////:
        //Archive
        /////////////////////////////////////////////////////////////////////////////:
        //1st try current entry
        
        //rebuild filepath
        fpath=[fpath stringByAppendingFormat:@"@%d",arcidx];
        if (subidx>=0) {
            fpath=[fpath stringByAppendingFormat:@"?%d",subidx];
        }
        
        if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
            //got stat
            if (rating==5) {
                return rating;
            }
        }
        
        //no stat, try at higher level
        //reset filepath
        fpath=[ModizFileHelper getFullCleanFilePath:[NSString stringWithString:filePath]];
        
        if (subidx>=0) {
            //1st case, subsongs available, try without taking current subsong into account
            //still take into account archive entry
            fpath=[fpath stringByAppendingFormat:@"@%d",arcidx];
            
            if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
                if (rating==5) {
                    return rating;
                }
            }
            //still no data, try at archive entry level
            fpath=[ModizFileHelper getFullCleanFilePath:[NSString stringWithString:filePath]];
            if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
                if (rating==5) {
                    return rating;
                }
            }
            return 0;
        }
        //2nd case, no subsong available
        //try global file
        if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
            if (rating==5) {
                return rating;
            }
        }
        return 0;
        
    } else if (subidx>=0) {
        /////////////////////////////////////////////////////////////////////////////:
        //No archive but Multisubsongs
        /////////////////////////////////////////////////////////////////////////////:
        // 1st, try  current entry
        fpath=[fpath stringByAppendingFormat:@"?%d",subidx];
        if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
            if (rating==5) {
                return rating;
            }
        }
        // no data, try at file level
        fpath=[ModizFileHelper getFilePathNoSubSong:filePath];
        if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
            if (rating==5) {
                return rating;
            }
        }
        //still no data
        return 0;
    }
    
    //simple file
    if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
        if (rating==5) {
            return rating;
        }
    }
    return 0;
}


