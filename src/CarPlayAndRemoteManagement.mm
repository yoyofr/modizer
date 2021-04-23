//
//  CarPlayAndRemoteManagement.m
//  modizer
//
//  Created by Yohann Magnien on 23/04/2021.
//

#import "CarPlayAndRemoteManagement.h"
#import "DetailViewControllerIphone.h"

@implementation CarPlayAndRemoteManagement

@synthesize detailViewController;

-(bool) initCarPlayAndRemote {
    ///////////////////////
    //Carplay
    ///////////////////////
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
    
    /*[cmdCenter.skipForwardCommand setEnabled:YES];
    [cmdCenter.skipForwardCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        
        
        NSNumber *seekTime=[[NSNumber alloc] initWithDouble:10*1000];
        
        
        [self.detailViewController performSelectorOnMainThread:@selector(skipForward:) withObject:seekTime waitUntilDone:YES];
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];*/
    
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
    [contMngr setNowPlayingIdentifiers:[NSArray arrayWithObject:@"item1"]];
    
    return TRUE;
}

- (void) initPlaylistList {
    
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
/*
/// Provides a content item for the provided identifier.
/// Provide nil if there is no content item corresponding to the identifier.
/// Provide an error if there is an error that will not allow content items
/// to be retrieved.
/// Client applications should always call the completion handler after loading
/// has finished, if this method is implemented.
- (void)contentItemForIdentifier:(NSString *)identifier completionHandler:(void(^)(MPContentItem *__nullable, NSError * __nullable))completionHandler {
    
    NSLog(@"yo3");
    if (completionHandler) completionHandler(nil,nil);
}

*/

/// Returns the number of child nodes at the specified index path. In a virtual
/// filesystem, this would be the number of files in a specific folder. An empty
/// index path represents the root node.
- (NSInteger)numberOfChildItemsAtIndexPath:(NSIndexPath *)indexPath {
    NSLog(@"indexPath len %lu",indexPath.length);
    if (indexPath.length) {
        return 0;
    }
    return 3;
}

/// Returns the content item at the specified index path. If the content item is
/// mutated after returning, its updated contents will be sent to MediaPlayer.
- (nullable MPContentItem *)contentItemAtIndexPath:(NSIndexPath *)indexPath {
    NSLog(@"%lu",indexPath.section);
    //if (indexPath.length) NSLog(@"%lu",indexPath.row);
    MPContentItem *item=[[MPContentItem alloc] initWithIdentifier:[NSString stringWithFormat:@"item%d",indexPath.section]];
    [item setTitle:[NSString stringWithFormat:@"item%lu",indexPath.section]];
    [item setPlayable:TRUE];
    [item setPlaybackProgress:0.1f*indexPath.section];
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
