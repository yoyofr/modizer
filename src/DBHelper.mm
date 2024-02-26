/*
 *  DBHelper.cpp
 *  modizer
 *
 *  Created by Yohann Magnien on 23/08/10.
 *  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
 *
 */
#include "DBHelper.h"

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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
		
		sprintf(sqltmp,"%s",[localPath cStringUsingEncoding:NSUTF8StringEncoding]);
//		printf("%s\n",sqltmp);
		
		if (adjusted) sprintf(sqlStatement,"SELECT fullpath FROM mod_file WHERE localpath like \"%s\"",sqltmp);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
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




void DBHelper::getFileStatsDBmod(NSString *name,NSString *fullpath,short int *playcount,signed char *rating,int *song_length,char *channels_nb,int *songs) {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
    
    if (name==nil) return;
	
	if (playcount) *playcount=0;
	if (rating) *rating=0;
	if (song_length) *song_length=0;
	if (channels_nb) *channels_nb=0;
	if (songs) *songs=0;
	
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
		
		sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[name UTF8String],[fullpath UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				if (playcount) *playcount=(short int)sqlite3_column_int(stmt, 0);
				if (rating) {
                    *rating=(signed char)sqlite3_column_int(stmt, 1);
                    if (*rating<0) *rating=0;
					if (*rating>5) *rating=5;
                }
				if (song_length) *song_length=(int)sqlite3_column_int(stmt, 2);
				if (channels_nb) *channels_nb=(char)sqlite3_column_int(stmt, 3);
				if (songs) *songs=(int)sqlite3_column_int(stmt, 4);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
	}
	sqlite3_close(db);
	
	pthread_mutex_unlock(&db_mutex);
}

void DBHelper::getFilesStatsDBmod(NSMutableArray *names,NSMutableArray *fullpaths,short int *playcountArray,signed char *ratingArray,int *song_lengthA,char *channels_nbA,int *songsA) {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
    
    
    if (names==nil) return ;
    if ([names count]==0) return ;
    
	int nb_entries=[names count];
	
	if (playcountArray) memset(playcountArray,0,sizeof(short int)*nb_entries);
	if (ratingArray) memset(ratingArray,0,sizeof(signed char)*nb_entries);
	if (song_lengthA) memset(song_lengthA,0,sizeof(int)*nb_entries);
	if (channels_nbA) memset(channels_nbA,0,sizeof(char)*nb_entries);
	if (songsA) memset(songsA,0,sizeof(int)*nb_entries);
	
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
		
		for (int i=0;i<nb_entries;i++) {
			sprintf(sqlStatement,"SELECT play_count,rating,length,channels_nb,songs FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[[names objectAtIndex:i] UTF8String],[[fullpaths objectAtIndex:i] UTF8String]);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					if (playcountArray) playcountArray[i]=(short int)sqlite3_column_int(stmt, 0);
					if (ratingArray) {
                        ratingArray[i]=(signed char)sqlite3_column_int(stmt, 1);
                        if (ratingArray[i]<0) ratingArray[i]=0;
                        if (ratingArray[i]>5) ratingArray[i]=5;
                    }
					if (song_lengthA) song_lengthA[i]=(int)sqlite3_column_int(stmt, 2);
					if (channels_nbA) channels_nbA[i]=(char)sqlite3_column_int(stmt, 3);
					if (songsA) songsA[i]=(int)sqlite3_column_int(stmt, 4);
				}
				sqlite3_finalize(stmt);
			} else {
				NSLog(@"ErrSQL : %d",err);
				break;
			}
		}
		
	}
	sqlite3_close(db);
	
	pthread_mutex_unlock(&db_mutex);
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
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
		
		sprintf(sqlStatement,"DELETE FROM user_stats WHERE fullpath=\"%s\"",[fullpath UTF8String]);
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
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
		
		sprintf(sqlStatement,"DELETE FROM user_stats WHERE fullpath like \"%s%%\"",[fullpath UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else {ret=0;NSLog(@"ErrSQL : %d",err);}
		
	}
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return ret;
}


void DBHelper::getFilesStatsDBmod(t_plPlaylist_entry *playlist,int nb_entries) {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
    
    if (playlist==NULL) return;
    if (nb_entries==0) return;
	
	for (int i=0;i<nb_entries;i++) {
		playlist[i].mPlaylistRating=0;
		playlist[i].mPlaylistCount=0;
	}
	
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
		
		for (int i=0;i<nb_entries;i++) {
			sprintf(sqlStatement,"SELECT play_count,rating FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[playlist[i].mPlaylistFilename UTF8String],[playlist[i].mPlaylistFilepath UTF8String]);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					playlist[i].mPlaylistRating=sqlite3_column_int(stmt, 1);
					playlist[i].mPlaylistCount=sqlite3_column_int(stmt, 0);
					if (playlist[i].mPlaylistRating<0) playlist[i].mPlaylistRating=0;
					else if (playlist[i].mPlaylistRating>5) playlist[i].mPlaylistRating=5;
				}
				sqlite3_finalize(stmt);
			} else {
				NSLog(@"ErrSQL : %d",err);
				break;
			}
		}
		
	}
	sqlite3_close(db);
	
	pthread_mutex_unlock(&db_mutex);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
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
		
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
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
    int entries_nb;
    int playcount,sng_length,channels,songs;
    
    if (fullpath==nil) return;
    
    //NSLog(@"upd avgrating: %@\n",fullpath);
    const char *tmp_str=[fullpath UTF8String];
    char tmp_str_copy[1024];
    int i=0,j=0;
    tmp_str_copy[j]=0;
    while (tmp_str[i]) {
        if (tmp_str[i]=='@') break;
        if (tmp_str[i]=='?') break;
        tmp_str_copy[j++]=tmp_str[i++];
        tmp_str_copy[j]=0;
    }
    NSString *fullpathCleaned=[NSString stringWithFormat:@"%s",tmp_str_copy];
    
    avg_rating=0;
    entries_nb=0;
    playcount=0;
    
    pthread_mutex_lock(&db_mutex);
    
    //1st get all related entries (archive entries & subsongs)
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT fullpath,play_count,rating,length,channels,songs FROM user_stats WHERE fullpath like \"%s%%\"",[fullpathCleaned UTF8String]);
        
        //printf("req: %s\n",sqlStatement);
        
        int fullpath_len=[fullpathCleaned length];
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int tmp_rating;
                char *tmp_fullpath;
                tmp_fullpath=(char*)sqlite3_column_text(stmt,0);
                
                if (strlen(tmp_fullpath)>fullpath_len) {
                
                    tmp_rating=(signed char)sqlite3_column_int(stmt, 2);
                
                    //printf("entry #%d, name: %s, rating: %d\n",entries_nb,tmp_fullpath,tmp_rating);
                
                    if (tmp_rating<0) tmp_rating=0;
                    if (tmp_rating>5) tmp_rating=5;
                    avg_rating+=tmp_rating;
                    //if (tmp_rating>0)
                    entries_nb++;
                } else {
                    //printf("skipping: %s\n",tmp_fullpath);
                    playcount=(int)sqlite3_column_int(stmt, 1);
                    sng_length=(int)sqlite3_column_int(stmt, 3);
                    channels=(int)sqlite3_column_int(stmt, 4);
                    songs=(int)sqlite3_column_int(stmt, 5);
                }
            }
            if (entries_nb&&songs) {
                /*if (avg_rating>0) {
                    avg_rating=avg_rating/entries_nb;
                    if (avg_rating==0) avg_rating=1;
                }*/
                if (avg_rating>0) {
                    avg_rating=abs(avg_rating/songs+1);
                    if (!avg_rating) avg_rating=1;
                }
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
    //2nd update rating based on average
    
    sprintf(sqlStatement,"DELETE FROM user_stats WHERE fullpath=\"%s\"",[fullpathCleaned UTF8String]);
    err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
    if (err==SQLITE_OK){
    } else NSLog(@"ErrSQL : %d",err);
        
    sprintf(sqlStatement,"INSERT INTO user_stats (name,fullpath,play_count,rating,length,channels,songs) SELECT \"%s\",\"%s\",%d,%d,%d,%d,%d",[[fullpathCleaned lastPathComponent] UTF8String],[fullpathCleaned UTF8String],playcount,avg_rating,sng_length,channels,songs);
    err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
    if (err==SQLITE_OK){
    } else NSLog(@"ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutex);
    
}

void DBHelper::updateFileStatsDBmod(NSString*name,NSString *fullpath,short int playcount,signed char rating) {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;	
	
    //NSLog(@"upFS pl %d ra %d %@ %@",playcount,rating,name,fullpath);
    
    if (name==nil) return;
    
    //NSLog(@"upd: %@/%@ played:%d rating:%d\n",name,[fullpath lastPathComponent],playcount,rating);
    
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
		
		sprintf(sqlStatement,"DELETE FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[name UTF8String],[fullpath UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
			
			sprintf(sqlStatement,"INSERT INTO user_stats (name,fullpath,play_count,rating) SELECT \"%s\",\"%s\",%d,%d",[name UTF8String],[fullpath UTF8String],playcount,rating);
			err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
			if (err==SQLITE_OK){
			} else NSLog(@"ErrSQL : %d",err);
				
				}
	sqlite3_close(db);
	
	pthread_mutex_unlock(&db_mutex);
}

void DBHelper::updateFileStatsDBmod(NSString*name,NSString *fullpath,short int playcount,signed char rating,int song_length,char channels_nb,int songs) {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
    
    //NSLog(@"upFS pl %d ra %d le %d ch %d sg %d %@ %@",playcount,rating,song_length,channels_nb,songs,name,fullpath);
    
    if (name==nil) return;
    
    //NSLog(@"updlong: %@/%@ played:%d rating:%d length:%d ...\n",name,[fullpath lastPathComponent],playcount,rating,song_length);
    
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
		sprintf(sqlStatement,"DELETE FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[name UTF8String],[fullpath UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
        
		sprintf(sqlStatement,"INSERT INTO user_stats (name,fullpath,play_count,rating,length,channels,songs) SELECT \"%s\",\"%s\",%d,%d,%d,%d,%d",[name UTF8String],[fullpath UTF8String],playcount,rating,song_length,channels_nb,songs);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
		
	}
	sqlite3_close(db);
	
	pthread_mutex_unlock(&db_mutex);
}



