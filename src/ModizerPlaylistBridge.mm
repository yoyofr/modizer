//
//  ModizerPlaylistBridge.mm
//  modizer
//
//  Created by Yohann Magnien David on 30/11/2025.
//

#import "ModizerPlaylistBridge.h"

// Only include the struct definition header (pure C)
#import "RootViewControllerPlaylist.h"
#import "RootViewControllerStruct.h"
#import <sqlite3.h>

// Forward declarations to avoid including headers with C++
@class AppDelegate_Phone;
@class RootViewControllerPlaylist;
@class DetailViewControllerIphone;

@implementation ModizerPlaylistInfo
@end

@implementation ModizerPlaylistBridge

+ (instancetype)sharedInstance {
    static ModizerPlaylistBridge *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[ModizerPlaylistBridge alloc] init];
    });
    return instance;
}

- (id)playlistViewController {
    id appDelegate = [[UIApplication sharedApplication] delegate];
    return [appDelegate valueForKey:@"playlistVC"];
}

- (id)detailViewController {
    id appDelegate = [[UIApplication sharedApplication] delegate];
    return [appDelegate valueForKey:@"detailViewControlleriPhone"];
}

- (NSArray<ModizerPlaylistInfo *> *)getAvailablePlaylists {
    id playlistVC = [self playlistViewController];
    if (!playlistVC) {
        return @[];
    }

    t_playlist_DB *plList = NULL;
    t_playlist_DB **plListPtr = &plList;  // Create pointer to pointer

    // Call loadPlayListsListFromDB: via performSelector
    SEL selector = NSSelectorFromString(@"loadPlayListsListFromDB:");
    NSMethodSignature *signature = [playlistVC methodSignatureForSelector:selector];
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
    [invocation setSelector:selector];
    [invocation setTarget:playlistVC];
    [invocation setArgument:&plListPtr atIndex:2];  // Pass address of pointer-to-pointer
    [invocation invoke];

    int count = 0;
    [invocation getReturnValue:&count];

    if (count <= 0 || !plList) {
        return @[];
    }

    NSMutableArray<ModizerPlaylistInfo *> *playlists = [NSMutableArray array];

    for (int i = 0; i < count; i++) {
        ModizerPlaylistInfo *info = [[ModizerPlaylistInfo alloc] init];
        info.playlistId = plList[i].pl_id;
        info.playlistName = [NSString stringWithUTF8String:plList[i].pl_name];
        info.playlistSize = plList[i].pl_size;
        [playlists addObject:info];
    }

    // Free the allocated memory
    for (int i = 0; i < count; i++) {
        if (plList[i].pl_name) {
            free(plList[i].pl_name);
        }
    }
    free(plList);

    return playlists;
}

- (BOOL)playPlaylistWithId:(int)playlistId startIndex:(int)startIndex {
    id detailVC = [self detailViewController];
    id playlistVC = [self playlistViewController];

    if (!detailVC || !playlistVC) {
        return NO;
    }

    // Load the playlist from database
    t_playlist *playlist = (t_playlist *)calloc(1, sizeof(t_playlist));
    if (!playlist) {
        return NO;
    }

    // Find and load the playlist with the given ID
    t_playlist_DB *plList = NULL;
    t_playlist_DB **plListPtr = &plList;

    SEL selector = NSSelectorFromString(@"loadPlayListsListFromDB:");
    NSMethodSignature *signature = [playlistVC methodSignatureForSelector:selector];
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
    [invocation setSelector:selector];
    [invocation setTarget:playlistVC];
    [invocation setArgument:&plListPtr atIndex:2];
    [invocation invoke];

    int count = 0;
    [invocation getReturnValue:&count];

    NSString *playlistIdStr = nil;
    if (count > 0 && plList) {
        for (int i = 0; i < count; i++) {
            if (plList[i].pl_id == playlistId) {
                playlistIdStr = [NSString stringWithFormat:@"%d", playlistId];
                playlist->playlist_name = [NSString stringWithUTF8String:plList[i].pl_name];
                break;
            }
        }

        // Free the allocated memory
        for (int i = 0; i < count; i++) {
            if (plList[i].pl_name) {
                free(plList[i].pl_name);
            }
        }
        free(plList);
    }

    if (!playlistIdStr) {
        free(playlist);
        return NO;
    }

    // Load playlist entries using the same method as the UI
    SEL loadSelector = NSSelectorFromString(@"loadPlayListsFromDB:intoPlaylist:");
    NSMethodSignature *loadSig = [playlistVC methodSignatureForSelector:loadSelector];
    NSInvocation *loadInv = [NSInvocation invocationWithMethodSignature:loadSig];
    [loadInv setSelector:loadSelector];
    [loadInv setTarget:playlistVC];
    [loadInv setArgument:&playlistIdStr atIndex:2];
    [loadInv setArgument:&playlist atIndex:3];
    [loadInv invoke];

    // Only proceed if we have entries
    if (playlist->nb_entries == 0) {
        free(playlist);
        return NO;
    }

    // Call play_listmodules:start_index: on detailVC on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        SEL playSelector = NSSelectorFromString(@"play_listmodules:start_index:");
        NSMethodSignature *playSig = [detailVC methodSignatureForSelector:playSelector];
        NSInvocation *playInv = [NSInvocation invocationWithMethodSignature:playSig];
        [playInv setSelector:playSelector];
        [playInv setTarget:detailVC];
        [playInv setArgument:&playlist atIndex:2];
        [playInv setArgument:&startIndex atIndex:3];
        [playInv retainArguments];
        [playInv invoke];
    });

    return YES;
}

- (BOOL)playBuiltinPlaylistWithId:(int)playlistId startIndex:(int)startIndex {
    id detailVC = [self detailViewController];
    id playlistVC = [self playlistViewController];
    
    if (!detailVC || !playlistVC) {
        return NO;
    }
    
    // Load the playlist from database
    t_playlist *playlist = (t_playlist *)calloc(1, sizeof(t_playlist));
    if (!playlist) {
        return NO;
    }
    
    if (playlistId==-1) { //Random picks
        
        NSMutableArray *arrayLabels=[[NSMutableArray alloc] init];
        NSMutableArray *arrayFullpaths=[[NSMutableArray alloc] init];
        int pl_entries;
        pl_entries=[playlistVC loadLocalFilesRandomPL:arrayLabels fullpaths:arrayFullpaths];
        
        playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Random picks",@"")];
        playlist->playlist_id=nil;
        playlist->nb_entries=pl_entries;
        for (int i=0;i<[arrayLabels count];i++) {
            playlist->entries[i].label=[arrayLabels objectAtIndex:i];
            playlist->entries[i].fullpath=[arrayFullpaths objectAtIndex:i];
            playlist->entries[i].ratings=-1;
        }
    } else if (playlistId==-2) { //Most played
        
        [playlistVC loadMostPlayedList:playlist];
        playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Most played",@"")];
        playlist->playlist_id=nil;
        
        // Only proceed if we have entries
        if (playlist->nb_entries == 0) {
            free(playlist);
            return NO;
        }
    } else if (playlistId==-3) { //Favorites
        
        [playlistVC loadMostPlayedList:playlist];
        playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Favorites",@"")];
        playlist->playlist_id=nil;
        
        // Only proceed if we have entries
        if (playlist->nb_entries == 0) {
            free(playlist);
            return NO;
        }
    }
    
    // Call play_listmodules:start_index: on detailVC on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        SEL playSelector = NSSelectorFromString(@"play_listmodules:start_index:");
        NSMethodSignature *playSig = [detailVC methodSignatureForSelector:playSelector];
        NSInvocation *playInv = [NSInvocation invocationWithMethodSignature:playSig];
        [playInv setSelector:playSelector];
        [playInv setTarget:detailVC];
        [playInv setArgument:&playlist atIndex:2];
        [playInv setArgument:&startIndex atIndex:3];
        [playInv retainArguments];
        [playInv invoke];
    });
    return YES;
}


- (BOOL)playPlaylistWithName:(NSString *)playlistName startIndex:(int)startIndex {
    NSArray<ModizerPlaylistInfo *> *playlists = [self getAvailablePlaylists];

    for (ModizerPlaylistInfo *info in playlists) {
        if ([info.playlistName isEqualToString:playlistName]) {
            return [self playPlaylistWithId:info.playlistId startIndex:startIndex];
        }
    }

    return NO;
}

@end
