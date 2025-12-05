//
//  ModizerPlaylistBridge.h
//  modizer
//
//  Created by Yohann Magnien David on 30/11/2025.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface ModizerPlaylistInfo : NSObject
@property (nonatomic, assign) int playlistId;
@property (nonatomic, copy) NSString *playlistName;
@property (nonatomic, assign) int playlistSize;
@end

@interface ModizerPlaylistBridge : NSObject

+ (instancetype)sharedInstance;

/// Returns array of ModizerPlaylistInfo
- (NSArray<ModizerPlaylistInfo *> *)getAvailablePlaylists;

/// Play a playlist by its ID
- (BOOL)playPlaylistWithId:(int)playlistId startIndex:(int)startIndex;

/// Play a playlist by its name
- (BOOL)playPlaylistWithName:(NSString *)playlistName startIndex:(int)startIndex;

/// Play a builtin playlist by its ID
/// playlistId: -1 is random picks, -2 is most played, -3 is favorites
- (BOOL)playBuiltinPlaylistWithId:(int)playlistId startIndex:(int)startIndex;

@end

NS_ASSUME_NONNULL_END
