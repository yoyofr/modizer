/*
 *  DBHelper.h
 *  modizer
 *
 *  Created by Yohann Magnien on 23/08/10.
 *  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
 *
 */
#ifndef st_DBHelper_h_
#define st_DBHelper_h_

typedef struct {
	NSString *mPlaylistFilename;
	NSString *mPlaylistFilepath;
	short int mPlaylistCount;  //internal, used for shuffle not for real playcount
	signed char mPlaylistRating;
	signed char cover_flag;
} t_plPlaylist_entry;


namespace DBHelper 
{
	void getFileStatsDBmod(NSString *name,NSString *fullpath,short int *playcount,signed char *rating,int *song_length=NULL,char *channels_nb=NULL,int *songs=NULL);    
	void getFilesStatsDBmod(NSMutableArray *names,NSMutableArray *fullpaths,short int *playcountArray,signed char *ratingArray,int *song_lengthA=NULL,char *channels_nbA=NULL,int *songsA=NULL);
    int deleteStatsDirDB(NSString *fullpath);
    int deleteStatsFileDB(NSString *fullpath);
	void getFilesStatsDBmod(t_plPlaylist_entry *playlist,int nb_entries);
    
	NSString *getLocalPathFromFullPath(NSString *fullPath);
	NSString *getFullPathFromLocalPath(NSString *localPath);
    void updateFileStatsAvgRatingDBmod(NSString *fullpath);
	void updateFileStatsDBmod(NSString*name,NSString *fullpath,short int playcount,signed char rating);
	void updateFileStatsDBmod(NSString*name,NSString *fullpath,short int playcount,signed char rating,int song_length,char channels_nb,int songs);
	
	int getNbFormatEntries();
	int getNbAuthorEntries();
	int getNbMODLANDFilesEntries();
	int getNbHVSCFilesEntries();
    int getNbASMAFilesEntries();
}

#endif
