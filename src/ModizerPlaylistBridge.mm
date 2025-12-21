//
//  ModizerPlaylistBridge.mm
//  modizer
//
//  Created by Yohann Magnien David on 30/11/2025.
//

#import "ModizerPlaylistBridge.h"
#import "ModizFileHelper.h"

// Only include the struct definition header (pure C)
#import "RootViewControllerPlaylist.h"
#import "RootViewControllerStruct.h"
#import "ModizerConstants.h"
#import <sqlite3.h>
#import <pthread.h>

// External mutex for database access (defined in other files)
extern pthread_mutex_t db_mutex;

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

- (id)tabBarController {
    id appDelegate = [[UIApplication sharedApplication] delegate];
    return [appDelegate valueForKey:@"tabBarC"];
}

- (NSArray<ModizerPlaylistInfo *> *)getAvailablePlaylists {
    // Access database directly without requiring view controller
    NSString *pathToDB = [NSString stringWithFormat:@"%@/%@",
                          [[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:@"Documents"],
                          DATABASENAME_USER];

    sqlite3 *db;
    NSMutableArray<ModizerPlaylistInfo *> *playlists = [NSMutableArray array];

    pthread_mutex_lock(&db_mutex);

    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK) {
        // Set pragmas
        sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);

        // Query playlists
        const char *sqlStatement = "SELECT id,name,num_files FROM playlists ORDER BY name";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ModizerPlaylistInfo *info = [[ModizerPlaylistInfo alloc] init];
                info.playlistId = sqlite3_column_int(stmt, 0);

                const char *name = (const char *)sqlite3_column_text(stmt, 1);
                info.playlistName = name ? [NSString stringWithUTF8String:name] : @"";

                info.playlistSize = sqlite3_column_int(stmt, 2);
                [playlists addObject:info];
            }
            sqlite3_finalize(stmt);
        }
    }

    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);

    return playlists;
}

- (BOOL)playPlaylistWithId:(int)playlistId startIndex:(int)startIndex {
    // Wait for app to be fully initialized (max 5 seconds)
    id detailVC = [self detailViewController];
    id playlistVC = [self playlistViewController];

    int attempts = 0;
    while ((!detailVC || !playlistVC) && attempts < 50) {
        [NSThread sleepForTimeInterval:0.1]; // 100ms
        detailVC = [self detailViewController];
        playlistVC = [self playlistViewController];
        attempts++;
    }

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

//    // Navigate to player view to bring app to foreground
//    id tabBarController = [self tabBarController];
//    if (tabBarController) {
//        [tabBarController performSelector:NSSelectorFromString(@"goToPlayerView")];
//    }

    // Call play_listmodules:start_index: on detailVC on main thread
    // Use dispatch_async with a small delay to ensure app is fully in foreground
    dispatch_async(dispatch_get_main_queue(), ^{
        // Small delay to ensure app is fully active
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
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
    });

    return YES;
}

- (BOOL)playBuiltinPlaylistWithId:(int)playlistId startIndex:(int)startIndex {
    // Wait for app to be fully initialized (max 5 seconds)
    id detailVC = [self detailViewController];
    id playlistVC = [self playlistViewController];

    int attempts = 0;
    while ((!detailVC || !playlistVC) && attempts < 50) {
        [NSThread sleepForTimeInterval:0.1]; // 100ms
        detailVC = [self detailViewController];
        playlistVC = [self playlistViewController];
        attempts++;
    }

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
        
        [playlistVC loadFavoritesList:playlist];
        playlist->playlist_name=[[NSString alloc] initWithFormat:NSLocalizedString(@"Favorites",@"")];
        playlist->playlist_id=nil;
        
        // Only proceed if we have entries
        if (playlist->nb_entries == 0) {
            free(playlist);
            return NO;
        }
    }
    
    // Navigate to player view to bring app to foreground
    id tabBarController = [self tabBarController];
    if (tabBarController) {
        [tabBarController performSelector:NSSelectorFromString(@"goToPlayerView")];
    }

    // Call play_listmodules:start_index: on detailVC on main thread
    // Use dispatch_async with a small delay to ensure app is fully in foreground
    dispatch_async(dispatch_get_main_queue(), ^{
        // Small delay to ensure app is fully active
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.3 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
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
