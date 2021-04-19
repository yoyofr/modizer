//
//  RootViewControllerStruct.h
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#import "ModizerConstants.h"

typedef struct {
	NSString *label;
	NSString *fullpath;    
	int song_length;
	int songs;
	short int playcount;
	char type; //0:dir, 1:file, 2:archives
	signed char rating;
	char channels_nb;
} t_local_browse_entry;

typedef struct {
	NSString *label;
	NSString *fullpath;
	int song_length;
	int songs;
	short int playcounts;
	signed char ratings;
	char channels_nb;
} t_playlist_entry;


typedef struct {
	NSString *playlist_name,*playlist_id;
	int nb_entries;
	t_playlist_entry entries[MAX_PL_ENTRIES];
} t_playlist;


typedef struct {
	NSString *label;
	int id_type;
	int id_author;
	int id_mod;
	int id_album;
	int filesize;
	int song_length;
	int songs;
	short int playcount;
	signed char downloaded;
	signed char rating;
	char channels_nb;
} t_db_browse_entry;

typedef struct {
	NSString *label;
	NSString *dir1,*dir2,*dir3,*dir4,*dir5;
	NSString *fullpath;
	NSString *id_md5;
	int filesize;
	int song_length;
	int songs;
	short int playcount;
	signed char downloaded;
	signed char rating;
	char channels_nb;
} t_dbHVSC_browse_entry;

typedef struct {
    NSString *label;
    NSString *fullpath;
    int filesize;
    signed char downloaded;
    signed char isFile;
} t_dbWEB_browse_entry;
