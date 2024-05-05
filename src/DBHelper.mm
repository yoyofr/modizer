/*
 *  DBHelper.cpp
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

NSMutableArray *DBHelper::getMissingPartsNameFromFilePath(NSString *localPath,NSString *ext) {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    NSMutableArray *result=[[NSMutableArray alloc] init];
    
    if (localPath==nil) return nil;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024],sqltmp[512];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        //removing the /Documents/MODLAND
        NSString *strTmpPath = [localPath stringByDeletingLastPathComponent];
        NSUInteger index = [strTmpPath rangeOfString:@"/"].location;
        strTmpPath = [strTmpPath substringFromIndex:index+1];
        index = [strTmpPath rangeOfString:@"/"].location;
        strTmpPath = [strTmpPath substringFromIndex:index+1];
        
        sprintf(sqltmp,"%s",[strTmpPath cStringUsingEncoding:NSUTF8StringEncoding]);
        
        sprintf(sqlStatement,"SELECT fullpath,localpath FROM mod_file WHERE localpath like \"%s/%%%s\"",sqltmp,[ext UTF8String]);
        //NSLog(@"sql: %s",sqlStatement);
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                NSString *localPath=[NSString stringWithFormat:@"%@/Documents/%@",[[NSBundle mainBundle] resourcePath],[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)]];
                //NSLog(@"checking %@",localPath);
                FILE *f=fopen([localPath UTF8String],"rb");
                if (!f) {
                    //NSLog(@"adding");
                    [result addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)]];
                    [result addObject:[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)]];
                }
                else fclose(f);
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
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
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024],sqltmp[512];
        int adjusted=0;
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqltmp,"%s",[localPath cStringUsingEncoding:NSUTF8StringEncoding]);
        //		printf("%s\n",sqltmp);
        
        if (adjusted) sprintf(sqlStatement,"SELECT fullpath FROM mod_file WHERE localpath LIKE \"%s\"",sqltmp);
        else sprintf(sqlStatement,"SELECT fullpath FROM mod_file WHERE localpath = \"%s\"",sqltmp);
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                result=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
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
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqltmp,"%s",[fullPath cStringUsingEncoding:NSNonLossyASCIIStringEncoding]);
        
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
        if (adjusted) sprintf(sqlStatement,"SELECT localpath FROM mod_file WHERE fullpath like \"%s\"",sqltmp);
        else sprintf(sqlStatement,"SELECT localpath FROM mod_file WHERE fullpath = \"%s\"",sqltmp);
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                result=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return result;
}





int DBHelper::getFileStatsDBmod(NSString *fullpath,short int *playcount,signed char *rating,signed char *avg_rating,int *song_length,char *channels_nb,int *songs) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
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
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT play_count,rating,avg_rating,length,channels,songs FROM user_stats WHERE fullpath=\"%s\"",[fullpath UTF8String]);
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
        } else NSLog(@"ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret;
}

int DBHelper::deleteStatsFileDB(NSString *fullpath) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err,ret;
    
    if (fullpath==nil) return -1;
    
    pthread_mutex_lock(&db_mutex);
    ret=1;
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"DELETE FROM user_stats WHERE fullpath LIKE \"%s%%\"",[fullpath UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else {ret=0;NSLog(@"ErrSQL : %d",err);}
        
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    return ret;
}
int DBHelper::deleteStatsDirDB(NSString *fullpath) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err,ret;
    
    if (fullpath==nil) return -1;
    
    pthread_mutex_lock(&db_mutex);
    ret=1;
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"DELETE FROM user_stats WHERE fullpath LIKE \"%s%%\"",[fullpath UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else {ret=0;NSLog(@"ErrSQL : %d",err);}
        
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
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT count(1) FROM mod_type");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret_int=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
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
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT count(1) FROM mod_author");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret_int=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
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
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT count(1) FROM hvsc_file");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret_int=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
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
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT count(1) FROM asma_file");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret_int=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
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
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT count(1) FROM mod_file");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ret_int=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret_int;
}

void DBHelper::updateFileStatsAvgRatingDBmod(NSString *fullpath) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
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
            } else NSLog(@"ErrSQL : %d",err);
            
            sprintf(sqlStatement,"SELECT name,fullpath,play_count,rating,length,channels,songs FROM user_stats WHERE fullpath like \"%s%%\"",[fullpathCleaned UTF8String]);
            
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
            } else NSLog(@"ErrSQL : %d",err);
            
            //2nd update rating based on average
            
            sprintf(sqlStatement,"DELETE FROM user_stats WHERE fullpath=\"%s\"",[fullpathCleaned UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
            } else NSLog(@"ErrSQL : %d",err);
            
            if (fname==NULL) fname=[fullpathCleaned lastPathComponent];
            
            sprintf(sqlStatement,"INSERT INTO user_stats (name,fullpath,play_count,rating,avg_rating,length,channels,songs) SELECT \"%s\",\"%s\",%d,%d,%d,%d,%d,%d",[fname UTF8String],[fullpathCleaned UTF8String],playcount,global_rating,avg_rating,sng_length,channels,songs);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
            } else NSLog(@"ErrSQL : %d",err);
            
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
            } else NSLog(@"ErrSQL : %d",err);
            
            sprintf(sqlStatement,"SELECT name,fullpath,play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath like \"%s%%\"",[fullpathCleaned UTF8String]);
            
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
            } else NSLog(@"ErrSQL : %d",err);
            
            //2nd update rating based on average
            
            sprintf(sqlStatement,"DELETE FROM user_stats WHERE fullpath=\"%s\"",[fullpathCleaned UTF8String]);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
            } else NSLog(@"ErrSQL : %d",err);
            
            if (fname==NULL) fname=[fullpathCleaned lastPathComponent];
            
            sprintf(sqlStatement,"INSERT INTO user_stats (name,fullpath,play_count,rating,avg_rating,length,channels,songs) SELECT \"%s\",\"%s\",%d,%d,%d,%d,%d,%d",[fname UTF8String],[fullpathCleaned UTF8String],playcount,global_rating,avg_rating,sng_length,channels,songs);
            err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
            if (err==SQLITE_OK){
            } else NSLog(@"ErrSQL : %d",err);
            
        }
        sqlite3_close(db);
        
        pthread_mutex_unlock(&db_mutex);
    }
    
    
}

int DBHelper::updateFileStatsDBmod(NSString *name,NSString *fullpath,short int playcount,signed char rating,signed char avg_rating,int song_length,signed char channels_nb,int songs) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    int ret=0;
    
    //NSLog(@"upFS pl %d ra %d avg %d le %d ch %d sg %d %@ %@",playcount,rating,avg_rating,song_length,channels_nb,songs,name,fullpath);
    
    if (name==NULL) return ret;
    
    //NSLog(@"updlong: %@/%@ played:%d rating:%d length:%d ...\n",name,[fullpath lastPathComponent],playcount,rating,song_length);
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = NORMAL;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT name,play_count,length,channels,songs,rating,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[fullpath UTF8String]);
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
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"DELETE FROM user_stats WHERE fullpath=\"%s\"",[fullpath UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        if (playcount==-1) playcount=0;
        if (rating==-1) rating=0;
        if (avg_rating==-1) avg_rating=0;
        
        sprintf(sqlStatement,"INSERT INTO user_stats (name,fullpath,play_count,rating,avg_rating,length,channels,songs) SELECT \"%s\",\"%s\",%d,%d,%d,%d,%d,%d",[name UTF8String],[fullpath UTF8String],playcount,rating,avg_rating,song_length,channels_nb,songs);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    return ret;
}

int DBHelper::updateRatingDBmod(NSString *fullpath,signed char rating) {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    int ret=0;
    
    //NSLog(@"upFS pl %d ra %d le %d ch %d sg %d %@ %@",playcount,rating,song_length,channels_nb,songs,name,fullpath);
    
    if (fullpath==nil) return ret;
    if ([fullpath isEqualToString:@""]) return ret;
    
    //NSLog(@"updlong: %@/%@ played:%d rating:%d length:%d ...\n",name,[fullpath lastPathComponent],playcount,rating,song_length);
    
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
        } else NSLog(@"ErrSQL : %d",err);
        
        
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT name,play_count,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[fullpath UTF8String]);
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
        } else NSLog(@"ErrSQL : %d",err);
        
        snprintf(sqlStatement,sizeof(sqlStatement),"DELETE FROM user_stats WHERE fullpath=\"%s\"",[fullpath UTF8String]);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        if (name==NULL) {
            name=(char*)strdup([[fullpath lastPathComponent] UTF8String]);
        }
        
        snprintf(sqlStatement,sizeof(sqlStatement),"INSERT INTO user_stats (name,fullpath,play_count,rating,avg_rating,length,channels,songs) SELECT \"%s\",\"%s\",%d,%d,%d,%d,%d,%d",name,[fullpath UTF8String],playcount,rating,avg_rating,song_length,channels_nb,songs);
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        if (name) free(name);
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    
    return ret;
}

bool dbhelper_cancel;

int DBHelper::cleanDB() {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    BOOL success;
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[256];
        char sqlStatement2[256];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        //---------------------------------------------------
        //Check if tables structure is up-to-date
        //---------------------------------------------------
        
        if (dbhelper_cancel) {
            sqlite3_close(db);
            pthread_mutex_unlock(&db_mutex);
            fileManager=nil;
            return -1;
        }
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
                        NSLog(@"Issue during add of avg_rating column in user_stats");
                    }
                }
                break;
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
        
        //First check that user_stats entries still exist
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT fullpath FROM user_stats");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                if (dbhelper_cancel) break;
                
                NSString *fullpath=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                //clean up for archive/multisong entries
                fullpath=[ModizFileHelper getFullCleanFilePath:fullpath];
                success = [fileManager fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent:fullpath]];
                if (!success) {//file does not exist
                    //NSLog(@"missing : %s",sqlite3_column_text(stmt, 0));
                    
                    snprintf(sqlStatement2,sizeof(sqlStatement2),"DELETE FROM user_stats WHERE fullpath LIKE \"%s%%\"",sqlite3_column_text(stmt, 0));
                    err=sqlite3_exec(db, sqlStatement2, NULL, NULL, NULL);
                    if (err!=SQLITE_OK) {
                        NSLog(@"Issue during delete of user_Stats");
                    }
                }
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
        if (dbhelper_cancel) {
            sqlite3_close(db);
            pthread_mutex_unlock(&db_mutex);
            fileManager=nil;
            return -1;
        }
        
        //Second check that playlist entries still exist
        snprintf(sqlStatement,sizeof(sqlStatement),"SELECT fullpath FROM playlists_entries");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                if (dbhelper_cancel) break;
                 
                NSString *fullpath=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                //clean up for archive/multisong entries
                fullpath=[ModizFileHelper getFullCleanFilePath:fullpath];
                success = [fileManager fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent:fullpath]];
                if (!success) {//file does not exist
                    NSLog(@"missing : %s",sqlite3_column_text(stmt, 0));
                    
                    sprintf(sqlStatement2,"DELETE FROM playlists_entries WHERE fullpath=\"%s\"",sqlite3_column_text(stmt, 0));
                    err=sqlite3_exec(db, sqlStatement2, NULL, NULL, NULL);
                    if (err!=SQLITE_OK) {
                        NSLog(@"Issue during delete of playlists_entries");
                    }
                }
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
        //update playlist table / entries
        snprintf(sqlStatement,sizeof(sqlStatement),"UPDATE playlists SET num_files=\
                (SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist)");
        err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
        if (err!=SQLITE_OK){
            NSLog(@"ErrSQL : %d",err);
        }
        
        //No defrag DB
        sprintf(sqlStatement2,"VACUUM");
        err=sqlite3_exec(db, sqlStatement2, NULL, NULL, NULL);
        if (err!=SQLITE_OK) {
            NSLog(@"Issue during VACUUM");
        }
    };
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
//                NSLog(@"Rating %d found for %@",rating,fpath);
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
//                    NSLog(@"Rating %d found for %@",rating,fpath);
                    return rating;
                }
            }
            //still no data, try at archive entry level
            fpath=[ModizFileHelper getFullCleanFilePath:[NSString stringWithString:filePath]];
            if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
                if (rating==5) {
//                    NSLog(@"Rating %d found for %@",rating,fpath);
                    return rating;
                }
            }
            //NSLog(@"no rating for %@",fpath);
            return 0;
        }
        //2nd case, no subsong available
        //try global file
        if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
            if (rating==5) {
//                NSLog(@"Rating %d found for %@",rating,fpath);
                return rating;
            }
        }
        //NSLog(@"no rating for %@",fpath);
        return 0;
        
    } else if (subidx>=0) {
        /////////////////////////////////////////////////////////////////////////////:
        //No archive but Multisubsongs
        /////////////////////////////////////////////////////////////////////////////:
        // 1st, try  current entry
        fpath=[fpath stringByAppendingFormat:@"?%d",subidx];
        if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
            if (rating==5) {
//                NSLog(@"Rating %d found for %@",rating,fpath);
                return rating;
            }
        }
        // no data, try at file level
        fpath=[ModizFileHelper getFilePathNoSubSong:filePath];
        if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
            if (rating==5) {
//                NSLog(@"Rating %d found for %@",rating,fpath);
                return rating;
            }
        }
        //still no data
//        NSLog(@"no rating for %@",fpath);
        return 0;
    }
    
    //simple file
    if (DBHelper::getFileStatsDBmod(fpath,NULL,&rating,NULL)) {
        if (rating==5) {
//            NSLog(@"Rating %d found for %@",rating,fpath);
            return rating;
        }
    }
    return 0;
}


