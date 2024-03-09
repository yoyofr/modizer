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
	int getFileStatsDBmod(NSString *fullpath,short int *playcount,signed char *rating,signed char *avg_rating,int *song_length=NULL,char *channels_nb=NULL,int *songs=NULL);
    int getRating(NSString *filePath,int arcidx,int subidx);
	int deleteStatsDirDB(NSString *fullpath);
    int deleteStatsFileDB(NSString *fullpath);
	
	NSString *getLocalPathFromFullPath(NSString *fullPath);
	NSString *getFullPathFromLocalPath(NSString *localPath);
    void updateFileStatsAvgRatingDBmod(NSString *fullpath);
	int updateFileStatsDBmod(NSString*name,NSString *fullpath,short int playcount=-1,signed char rating=-1,signed char avg_rating=-1,int song_length=-1,signed char channels_nb=-1,int songs=-1);
    NSMutableArray *getMissingPartsNameFromFilePath(NSString *localPath,NSString *ext);

    int updateRatingDBmod(NSString *fullpath,signed char rating);
	
	int getNbFormatEntries();
	int getNbAuthorEntries();
	int getNbMODLANDFilesEntries();
	int getNbHVSCFilesEntries();
    int getNbASMAFilesEntries();

    int cleanDB();
}

#endif
