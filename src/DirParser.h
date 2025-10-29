//
//  DirParser.h
//  modizer
//
//  Created by Yohann Magnien David on 26/10/2025.
//

#import <Foundation/Foundation.h>

#include <projectM-4/projectM.h>

@interface FileNode : NSObject

@property (nonatomic, strong) NSString *name;
@property (nonatomic, strong) NSString *path;
@property (nonatomic, assign) BOOL isSelected;
@property (nonatomic, assign) int selectedChildren;
@property (nonatomic, assign) BOOL isFullySelected;
@property (nonatomic, assign) BOOL isMatchingFilter;
@property (nonatomic, assign) BOOL isDirectory;
@property (nonatomic, assign) BOOL isMissing;
@property (nonatomic, assign) BOOL isFavorite;
@property (nonatomic, assign) BOOL shouldPropagateStatus;
@property (nonatomic, assign) unsigned long long fileSize;
@property (nonatomic, strong) NSDate *modificationDate;
@property (nonatomic, strong) NSMutableArray<FileNode *> *children;

- (instancetype)initWithPath:(NSString *)path;

- (bool) filterNodes:(NSString *)pattern filterDir:(bool)filterDir;
- (void) flattenNode:(FileNode *)node selected:(bool)filterSelected favorite:(bool)filterFav intoArray:(NSMutableArray<FileNode *> *)array;
- (NSArray*) getSelectedPlaylist;
- (NSArray*) getFavoritePlaylist;


@end

@interface MDZPlaylist : NSObject

@property (nonatomic, strong) NSMutableArray *items;
@property (nonatomic, strong) NSMutableArray *history;
@property (nonatomic, strong) NSString *curEntryLbl;
@property (nonatomic, strong) NSString *playlistName;
@property (nonatomic, assign) projectm_handle pmh;
@property (nonatomic, assign) int position;
@property (nonatomic, assign) int size;
@property (nonatomic, assign) bool shuffle;
@property (nonatomic, assign) bool lastFailed;

- (instancetype)init:(projectm_handle)pmh name:(NSString*)name;
- (instancetype)initWithArray:(NSArray*)array pmh:(projectm_handle)pmh name:(NSString*)name;
- (void)setItems:(NSArray*)array;
- (void)addItems:(NSArray*)array;
- (void)setShuffle:(bool)active;
- (void)next:(bool)cut;
- (void)prev:(bool)cut;
- (void)last:(bool)cut;
- (int)getPos;
- (void)setPos:(int)pos cut:(bool)cut;
- (void)remove:(int)index;
- (void)clear;
- (const char*)getTitle:(int)index;
- (const char*)getPath:(int)index;
- (const char*)getCurLabel;
- (int)getSize;
- (void)loadCurrentPreset:(bool)cut;
- (void)loadIdlePreset;
- (const char *)getPresetCleanTitle:(int)index;
- (const char *)getCurPresetCleanTitle;

/*! \brief Save playlist
 *
 *  Save playlist
 *
 *  \return 0 on success
*                     <0 in case of erorr
 */
- (int)savePlaylist;

/*! \brief Load playlist
 *
 *  Load playlist and check / available files
 *
 *  \return 0 full succes. Number of missing entries after filesystem check on partial success (playlist loaded, but some files are missing)
 *          <0 in case of error
 */
- (int)loadPlaylist;

@end

@interface DirParser : NSObject

@property (nonatomic, assign) BOOL includeHiddenFiles;
@property (nonatomic, strong) NSString *filterExt;
@property (nonatomic, assign) NSInteger maxDepth;

- (FileNode *)parseDirectoryAtPath:(NSString *)path error:(NSError **)error;
- (FileNode *)parseDirectoryAtPath:(NSString *)path depth:(NSInteger)depth error:(NSError **)error;
- (NSArray<FileNode *> *)flattenTree:(FileNode *)root;

@end
