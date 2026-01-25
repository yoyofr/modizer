//
//  RootViewControllerStruct.h
//  modizer
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#import "ModizerConstants.h"
#include <string.h>

typedef struct {
	NSString *label;
    NSString *altlabel;
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
    char *pl_name;
    int pl_id;
    int pl_size;
} t_playlist_DB;


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
    NSAttributedString *labelAttr;
    NSString *fullpath;
    NSString *URL;
    NSString *img_URL;
    NSString *info;
    NSAttributedString *infoAttr;
    double webRating;
    float ratingNb;
    int entries_nb;
    int isFile;
    char url_type;
    signed char downloaded;
    int song_length;
    int songs;
    short int playcount;
    signed char rating;
    char channels_nb;    
    bool has_letter_index;
    NSArray *extra_index;
} t_WEB_browse_entry;
