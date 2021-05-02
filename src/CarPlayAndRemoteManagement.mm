//
//  CarPlayAndRemoteManagement.m
//  modizer
//
//  Created by Yohann Magnien on 23/04/2021.
//

#import "CarPlayAndRemoteManagement.h"
#import "DetailViewControllerIphone.h"

#include <pthread.h>

@implementation CarPlayAndRemoteManagement

@synthesize detailViewController;
@synthesize rootVCLocalB;

-(void) flushMainLoop {
    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate date]];
}

-(void) refreshMPItems {
    MPPlayableContentManager *contMngr=[MPPlayableContentManager sharedContentManager];
    if (plArray) {
        MPContentItem *item=(MPContentItem*)([plArray objectAtIndex:0]);
        if (detailViewController.mPlaylist_size) {
            [item setTitle:[NSString stringWithFormat:NSLocalizedString(@"Now playing(%d)",@""),detailViewController.mPlaylist_size]];
            [item setPlayable:TRUE];
            [item setPlaybackProgress:(float)(detailViewController.mPlaylist_pos+1)/(float)(detailViewController.mPlaylist_size)];
        } else {
            [item setTitle:[NSString stringWithFormat:NSLocalizedString(@"Nothing playing",@"")]];
            [item setPlayable:FALSE];
        }
        if (detailViewController.mIsPlaying) {
            if ([detailViewController.mplayer isPaused]) [contMngr setNowPlayingIdentifiers:[NSArray arrayWithObject:@""]];
            else [contMngr setNowPlayingIdentifiers:[NSArray arrayWithObject:@"pl_NP"]];
        }
    }
}

-(bool) initCarPlayAndRemote {
    ///////////////////////
    //Carplay data
    ///////////////////////
    t_playlist_DB *plList;
    int plListsize=[rootVCLocalB loadPlayListsListFromDB:&plList];
    
    plArray=[[NSMutableArray alloc] init];
    
    //Integrated playlists
    if (1/*detailViewController.mPlaylist_size*/) {
        MPContentItem *item=[[MPContentItem alloc] initWithIdentifier:@"pl_NP"];
        
        if (detailViewController.mPlaylist_size) {
            [item setTitle:[NSString stringWithFormat:NSLocalizedString(@"Now playing(%d)",@""),detailViewController.mPlaylist_size]];
            [item setPlayable:TRUE];
            [item setPlaybackProgress:(float)(detailViewController.mPlaylist_pos+1)/(float)(detailViewController.mPlaylist_size)];
        } else {
            [item setTitle:[NSString stringWithFormat:NSLocalizedString(@"Nothing playing",@"")]];
            [item setPlayable:FALSE];
        }
        
        [plArray addObject:item];
    }
    if (1) {
        MPContentItem *item=[[MPContentItem alloc] initWithIdentifier:@"pl_MP"];
        [item setTitle:[NSString stringWithFormat:NSLocalizedString(@"Most played",@"")]];
        [item setPlayable:TRUE];
        
        [plArray addObject:item];
    }
    if (1) {
        MPContentItem *item=[[MPContentItem alloc] initWithIdentifier:@"pl_FV"];
        [item setTitle:[NSString stringWithFormat:NSLocalizedString(@"Favorites",@"")]];
        [item setPlayable:TRUE];
        
        [plArray addObject:item];
    }
    if (1) {
        MPContentItem *item=[[MPContentItem alloc] initWithIdentifier:@"pl_RD"];
        [item setTitle:[NSString stringWithFormat:NSLocalizedString(@"Random",@"")]];
        [item setPlayable:TRUE];
        [item setPlaybackProgress:(float)(detailViewController.mPlaylist_pos)/(float)(detailViewController.mPlaylist_size)];
        
        [plArray addObject:item];
    }
        
    //free playlists list
    for (int i=0;i<plListsize;i++) {
        MPContentItem *item=[[MPContentItem alloc] initWithIdentifier:[NSString stringWithFormat:@"pl_#%d",plList[i].pl_id]];
        [item setTitle:[NSString stringWithFormat:@"%s(%d)",plList[i].pl_name,plList[i].pl_size]];
        [item setPlayable:TRUE];
        [item setPlaybackProgress:0];
        
        [plArray addObject:item];
        
        mdz_safe_free(plList[i].pl_name);
    }
    mdz_safe_free(plList);
    
    ////////////////////////
    //Remote control mngt
    //////////////////////////
    MPRemoteCommandCenter *cmdCenter=[MPRemoteCommandCenter sharedCommandCenter];
    
    NSArray *commands = @[cmdCenter.playCommand, cmdCenter.pauseCommand, cmdCenter.nextTrackCommand, cmdCenter.previousTrackCommand, cmdCenter.bookmarkCommand, cmdCenter.changePlaybackPositionCommand, cmdCenter.changePlaybackRateCommand, cmdCenter.dislikeCommand, cmdCenter.enableLanguageOptionCommand, cmdCenter.likeCommand, cmdCenter.ratingCommand, cmdCenter.seekBackwardCommand, cmdCenter.seekForwardCommand, cmdCenter.skipBackwardCommand, cmdCenter.skipForwardCommand, cmdCenter.stopCommand, cmdCenter.togglePlayPauseCommand];

    
    for (MPRemoteCommand *command in commands) {
        [command removeTarget:nil];
        [command setEnabled:NO];
    }
    
    [cmdCenter.playCommand setEnabled:YES];
    [cmdCenter.playCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        [self.detailViewController performSelectorOnMainThread:@selector(playPushed:) withObject:nil waitUntilDone:YES];
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    [cmdCenter.pauseCommand setEnabled:YES];
    [cmdCenter.pauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        [self.detailViewController performSelectorOnMainThread:@selector(pausePushed:) withObject:nil waitUntilDone:YES];
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    [cmdCenter.previousTrackCommand setEnabled:YES];
    [cmdCenter.previousTrackCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        [self.detailViewController performSelectorOnMainThread:@selector(playPrevSub) withObject:nil waitUntilDone:YES];
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    [cmdCenter.nextTrackCommand setEnabled:YES];
    [cmdCenter.nextTrackCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        [self.detailViewController performSelectorOnMainThread:@selector(playNextSub) withObject:nil waitUntilDone:YES];
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    [cmdCenter.changePlaybackPositionCommand setEnabled:YES];
    [cmdCenter.changePlaybackPositionCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        
        
        NSNumber *seekTime=[[NSNumber alloc] initWithDouble:((MPChangePlaybackPositionCommandEvent*)event).positionTime*1000];
        [self.detailViewController performSelectorOnMainThread:@selector(seek:) withObject:seekTime waitUntilDone:YES];
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    
    [cmdCenter.seekForwardCommand setEnabled:YES];
    [cmdCenter.seekForwardCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        
        if ( ((MPSeekCommandEvent*)event).type==MPSeekCommandEventTypeBeginSeeking) {
            [self.detailViewController performSelectorOnMainThread:@selector(playNext) withObject:nil waitUntilDone:YES];
            [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        }
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    
    [cmdCenter.seekBackwardCommand setEnabled:YES];
    [cmdCenter.seekBackwardCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        if ( ((MPSeekCommandEvent*)event).type==MPSeekCommandEventTypeBeginSeeking) {
            [self.detailViewController performSelectorOnMainThread:@selector(playPrev) withObject:nil waitUntilDone:YES];
            [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        }
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    
    [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
    
    MPPlayableContentManager *contMngr=[MPPlayableContentManager sharedContentManager];
    [contMngr setDelegate:self];
    [contMngr setDataSource:self];
    [contMngr reloadData];
    if (detailViewController.mIsPlaying) {
        if ([detailViewController.mplayer isPaused]) [contMngr setNowPlayingIdentifiers:[NSArray arrayWithObject:@""]];
        else [contMngr setNowPlayingIdentifiers:[NSArray arrayWithObject:@"pl_NP"]];
    }
    
    
    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 1.0f target:self selector:@selector(refreshMPItems) userInfo:nil repeats: YES]; //1 times/second
    
    return TRUE;
}

///////////////////////
//Carplay
///////////////////////

- (void)playableContentManager:(MPPlayableContentManager *)contentManager didUpdateContext:(MPPlayableContentManagerContext *)context {
    NSLog(@"yo");
}


/*/// Tells the data source to begin loading content items that are children of the
/// item specified by indexPath. This is provided so that applications can begin
/// asynchronous batched loading of content before MediaPlayer begins asking for
/// content items to display.
/// Client applications should always call the completion handler after loading
/// has finished, if this method is implemented.
- (void)beginLoadingChildItemsAtIndexPath:(NSIndexPath *)indexPath completionHandler:(void(^)(NSError * __nullable))completionHandler {
    NSLog(@"yo1");
    if (completionHandler) completionHandler(nil);
}
*/
/// Tells MediaPlayer whether the content provided by the data source supports
/// playback progress as a property of its metadata.
/// If this method is not implemented, MediaPlayer will assume that progress is
/// not supported for any content items.
- (BOOL)childItemsDisplayPlaybackProgressAtIndexPath:(NSIndexPath *)indexPath {
    //NSLog(@"yo2");
    return YES;
}

/// Provides a content item for the provided identifier.
/// Provide nil if there is no content item corresponding to the identifier.
/// Provide an error if there is an error that will not allow content items
/// to be retrieved.
/// Client applications should always call the completion handler after loading
/// has finished, if this method is implemented.
- (void)contentItemForIdentifier:(NSString *)identifier completionHandler:(void(^)(MPContentItem *__nullable, NSError * __nullable))completionHandler {
    
    NSLog(@"yo3 %@",identifier);
    if (completionHandler) completionHandler(nil,nil);
}



/// Returns the number of child nodes at the specified index path. In a virtual
/// filesystem, this would be the number of files in a specific folder. An empty
/// index path represents the root node.
- (NSInteger)numberOfChildItemsAtIndexPath:(NSIndexPath *)indexPath {
    //NSLog(@"indexPath len %lu",indexPath.length);
    if (indexPath.length) {
        return 0;
    }
    return [plArray count];
}

/// Returns the content item at the specified index path. If the content item is
/// mutated after returning, its updated contents will be sent to MediaPlayer.
- (nullable MPContentItem *)contentItemAtIndexPath:(NSIndexPath *)indexPath {
    //NSLog(@"%lu",indexPath.section);
    
    MPContentItem *item=(MPContentItem*)([plArray objectAtIndex:indexPath.section]);
    return item;
}

/// This method is called when a media player interface wants to play a requested
/// content item. The application should call the completion handler with an
/// appropriate error if there was an error beginning playback for the item.

- (void)playableContentManager:(MPPlayableContentManager *)contentManager initiatePlaybackOfContentItemAtIndexPath:(NSIndexPath *)indexPath completionHandler:(void(^)(NSError * __nullable))completionHandler {
    
    [self.detailViewController performSelectorOnMainThread:@selector(playPushed:) withObject:nil waitUntilDone:YES];
    completionHandler(nil);
}


//////////////////////////


@end
