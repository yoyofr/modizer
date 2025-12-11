//
//  CarPlayAndRemoteManagement.m
//  modizer
//
//  Created by Yohann Magnien on 23/04/2021.
//

#import "CarPlayAndRemoteManagement.h"
#import "DetailViewControllerIphone.h"
#import "MDZCarPlaySceneDelegate.h"
#import <CarPlay/CarPlay.h>

#include <pthread.h>

@implementation CarPlayAndRemoteManagement

@synthesize detailViewController;
@synthesize rootVCLocalB;
@synthesize carPlaySceneDelegate;

-(void) flushMainLoop {
    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate date]];
//    NSDate* futureDate = [NSDate dateWithTimeInterval:0.001f sinceDate:[NSDate date]];
//    [[NSRunLoop currentRunLoop] runUntilDate:futureDate];
}

-(void) updatePlaybackProgress {
    // Update the Now Playing info for progress bar updates
    [detailViewController updMediaCenter];

    // Check if track position has changed and update playlist display if needed
    static int lastPlaylistPos = -1;
    static int lastPlaylistSize = -1;

    int currentPos = detailViewController.mPlaylist_pos;
    int currentSize = detailViewController.mPlaylist_size;

    // Only refresh the template if track position or playlist size changed
    if (currentPos != lastPlaylistPos || currentSize != lastPlaylistSize) {
        lastPlaylistPos = currentPos;
        lastPlaylistSize = currentSize;
        [self refreshMPItems];
    }
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

    // Update the modern CarPlay template if available
    // This is only called when the playlist actually changes, not every second
    if (self.carPlaySceneDelegate) {
        [self refreshNowPlayingButtons];
        
        [self.carPlaySceneDelegate updatePlaylistsDisplay];
    }
}

// Helper method to invert alpha channel (opaque becomes transparent and vice-versa)
- (UIImage *)invertImage:(UIImage *)image {
    CGSize size = image.size;
    CGFloat scale = image.scale;

    // Create a bitmap context
    size_t width = size.width * scale;
    size_t height = size.height * scale;
    size_t bytesPerRow = width * 4;

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    unsigned char *rawData = (unsigned char *)calloc(height * bytesPerRow, sizeof(unsigned char));

    CGContextRef bitmapContext = CGBitmapContextCreate(rawData, width, height, 8, bytesPerRow, colorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(colorSpace);

    // Draw the original image
    CGContextDrawImage(bitmapContext, CGRectMake(0, 0, width, height), image.CGImage);

    // Invert the alpha channel: for each pixel, set alpha = 255 - alpha, keep RGB as black
    for (size_t i = 0; i < width * height; i++) {
        size_t offset = i * 4;
        unsigned char alpha = rawData[offset + 3];

        // Set RGB to black and invert alpha
        rawData[offset + 0] = 0;  // R
        rawData[offset + 1] = 0;  // G
        rawData[offset + 2] = 0;  // B
        rawData[offset + 3] = 255 - alpha;  // Inverted alpha
    }

    // Create image from modified data
    CGImageRef imageRef = CGBitmapContextCreateImage(bitmapContext);
    UIImage *invertedImage = [UIImage imageWithCGImage:imageRef scale:scale orientation:image.imageOrientation];

    CGImageRelease(imageRef);
    CGContextRelease(bitmapContext);
    free(rawData);

    return invertedImage;
}

- (void)refreshNowPlayingButtons {
    if (@available(iOS 14.0, *)) {
        CPNowPlayingTemplate *nowPlayingTemplate = [CPNowPlayingTemplate sharedTemplate];

        NSMutableArray *buttons = [NSMutableArray array];

        // Loop/Repeat button (3 levels: 0=off, 1=all, 2=one)
        CPNowPlayingRepeatButton *repeatButton = [[CPNowPlayingRepeatButton alloc] initWithHandler:^(CPNowPlayingButton * _Nonnull button) {
            [self.detailViewController performSelectorOnMainThread:@selector(changeLoopMode) withObject:nil waitUntilDone:YES];
            [self refreshNowPlayingButtons];
        }];
        repeatButton.enabled = YES;
        [buttons addObject:repeatButton];

        // Shuffle button (3 levels: 0=off, 1=mode1, 2=mode2)
        UIImage *shuffleImage;
        switch (self.detailViewController.mShuffle) {
            case 0:
                // Shuffle off - just the symbol
                shuffleImage = [UIImage systemImageNamed:@"shuffle"];
                break;
            case 1:
             {
                // Shuffle mode 2 - shuffle + number "1" as text
                UIImage *shuffleIcon = [UIImage systemImageNamed:@"shuffle"];

                UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(120, 120)];
                UIImage *renderedImage = [renderer imageWithActions:^(UIGraphicsImageRendererContext * _Nonnull context) {
                    // Draw shuffle symbol in black
                    UIImage *tintedShuffle = [shuffleIcon imageWithTintColor:[UIColor blackColor] renderingMode:UIImageRenderingModeAlwaysOriginal];
                    [tintedShuffle drawInRect:CGRectMake(-10, 10, 140, 100)];

                    // Draw the number "1" as text
                    NSString *text = @"1";
                    UIFont *font = [UIFont boldSystemFontOfSize:50];
                    NSDictionary *attributes = @{
                        NSFontAttributeName: font,
                        NSForegroundColorAttributeName: [UIColor blackColor]
                    };

                    CGSize textSize = [text sizeWithAttributes:attributes];
                    CGPoint textPosition = CGPointMake(5, 60-30);
                    [text drawAtPoint:textPosition withAttributes:attributes];
                }];
                shuffleImage = renderedImage;
                break;
            }
            case 2:
            default:{
                // Shuffle mode 1 - Inverted alpha in template mode
                UIImage *baseImage = [UIImage systemImageNamed:@"shuffle"];
                UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(120, 120)];
                UIImage *renderedImage = [renderer imageWithActions:^(UIGraphicsImageRendererContext * _Nonnull context) {
                    // Draw symbol in black
                    UIImage *tintedImage = [baseImage imageWithTintColor:[UIColor blackColor] renderingMode:UIImageRenderingModeAlwaysOriginal];
                    [tintedImage drawInRect:CGRectMake(-10, 10, 140, 100)];
                }];
                // Invert alpha then apply template mode
                //UIImage *invertedImage = [self invertImage:renderedImage];
                shuffleImage = renderedImage;//[invertedImage imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
                break;
            }
        }
        CPNowPlayingImageButton *shuffleButton = [[CPNowPlayingImageButton alloc] initWithImage:shuffleImage
                                                                                         handler:^(CPNowPlayingButton * _Nonnull button) {
            [self.detailViewController performSelectorOnMainThread:@selector(shuffle) withObject:nil waitUntilDone:YES];
            [self refreshNowPlayingButtons];
        }];
        // Mark button as selected when shuffle is active (mode 1 or 2)
        shuffleButton.selected = (self.detailViewController.mShuffle != 0);
        [buttons addObject:shuffleButton];

        // Favorite button
        if (self.detailViewController.mPlaylist_size > 0) {
            // Get rating from current playlist entry
            signed char rating = self.detailViewController.mPlaylist[self.detailViewController.mPlaylist_pos].mPlaylistRating;

            // Choose icon based on whether it's a favorite (rating = 5)
            UIImage *favoriteImage = (rating == 5) ? [UIImage systemImageNamed:@"heart.fill"] : [UIImage systemImageNamed:@"heart"];

            CPNowPlayingImageButton *favoriteButton = [[CPNowPlayingImageButton alloc] initWithImage:favoriteImage
                                                                                             handler:^(CPNowPlayingButton * _Nonnull button) {
                if (rating == 5) {
                    // Remove from favorites
                    [self.detailViewController performSelectorOnMainThread:@selector(cmdDislike) withObject:nil waitUntilDone:YES];
                } else {
                    // Add to favorites
                    [self.detailViewController performSelectorOnMainThread:@selector(cmdLike) withObject:nil waitUntilDone:YES];
                }
                [self refreshNowPlayingButtons];
            }];
            [buttons addObject:favoriteButton];
        }

        // Update the CarPlay template with the new buttons
        [nowPlayingTemplate updateNowPlayingButtons:buttons];
    }
}

-(bool) initCarPlayAndRemote {
    ///////////////////////
    //Carplay data
    ///////////////////////
    t_playlist_DB *plList;
    plList=NULL;
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
        int nb_entries=[rootVCLocalB getMostPlayedCountFromDB];
        MPContentItem *item=[[MPContentItem alloc] initWithIdentifier:@"pl_MP"];
        [item setTitle:[NSString stringWithFormat:NSLocalizedString(@"Most played (%d)",@""),nb_entries]];
        if (nb_entries) [item setPlayable:TRUE];
        else [item setPlayable:FALSE];
        
        [plArray addObject:item];
    }
    if (1) {
        int nb_entries=[rootVCLocalB getFavoritesCountFromDB];
        MPContentItem *item=[[MPContentItem alloc] initWithIdentifier:@"pl_FV"];
        [item setTitle:[NSString stringWithFormat:NSLocalizedString(@"Favorites (%d)",@""),nb_entries]];
        if (nb_entries) [item setPlayable:TRUE];
        else [item setPlayable:FALSE];
        
        [plArray addObject:item];
    }
    if (1) { //YOYOFR: To review perf / OPTIM!
        //int nb_entries=[rootVCLocalB getLocalFilesCount];
        MPContentItem *item=[[MPContentItem alloc] initWithIdentifier:@"pl_RD"];
        [item setTitle:NSLocalizedString(@"Random picks",@"")];
        /*if (nb_entries)*/ [item setPlayable:TRUE];
        //else [item setPlayable:FALSE];
        
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
        [self.detailViewController performSelectorOnMainThread:@selector(playPushed) withObject:nil waitUntilDone:YES];
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    [cmdCenter.pauseCommand setEnabled:YES];
    [cmdCenter.pauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        [self.detailViewController performSelectorOnMainThread:@selector(pausePushed) withObject:nil waitUntilDone:YES];
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    [cmdCenter.togglePlayPauseCommand setEnabled:YES];
    [cmdCenter.togglePlayPauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        if (self.detailViewController.mPaused) [self.detailViewController performSelectorOnMainThread:@selector(playPushed) withObject:nil waitUntilDone:YES];
        else [self.detailViewController performSelectorOnMainThread:@selector(pausePushed) withObject:nil waitUntilDone:YES];
                
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    
    [cmdCenter.likeCommand setEnabled:NO];
    [cmdCenter.likeCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        
        [self.detailViewController performSelectorOnMainThread:@selector(cmdLike) withObject:nil waitUntilDone:YES];
                
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    
    [cmdCenter.dislikeCommand setEnabled:NO];
    [cmdCenter.dislikeCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        
        [self.detailViewController performSelectorOnMainThread:@selector(cmdDislike) withObject:nil waitUntilDone:YES];
                
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    
    cmdCenter.likeCommand.localizedTitle=NSLocalizedString(@"Add to favorites",@"");
    cmdCenter.likeCommand.localizedShortTitle=NSLocalizedString(@"Add to favorites",@"");
    cmdCenter.dislikeCommand.localizedTitle=NSLocalizedString(@"Remove from favorites",@"");
    cmdCenter.dislikeCommand.localizedShortTitle=NSLocalizedString(@"Remove from favorites",@"");
    
    
    [cmdCenter.changeRepeatModeCommand setEnabled:YES];
    [cmdCenter.changeRepeatModeCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        [self.detailViewController performSelectorOnMainThread:@selector(changeLoopMode) withObject:nil waitUntilDone:YES];
        switch (self.detailViewController.mLoopMode) {
            case 0:
                cmdCenter.changeRepeatModeCommand.currentRepeatType=MPRepeatTypeOff;
                break;
            case 1:
                cmdCenter.changeRepeatModeCommand.currentRepeatType=MPRepeatTypeAll;
                break;
            case 2:
                cmdCenter.changeRepeatModeCommand.currentRepeatType=MPRepeatTypeOne;
                break;
        }
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    switch (self.detailViewController.mLoopMode) {
        case 0:
            cmdCenter.changeRepeatModeCommand.currentRepeatType=MPRepeatTypeOff;
            break;
        case 1:
            cmdCenter.changeRepeatModeCommand.currentRepeatType=MPRepeatTypeAll;
            break;
        case 2:
            cmdCenter.changeRepeatModeCommand.currentRepeatType=MPRepeatTypeOne;
            break;
    }
    
    
    [cmdCenter.changeShuffleModeCommand setEnabled:YES];
    [cmdCenter.changeShuffleModeCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        [self.detailViewController performSelectorOnMainThread:@selector(shuffle) withObject:nil waitUntilDone:YES];
        
        if (self.detailViewController.mShuffle) cmdCenter.changeShuffleModeCommand.currentShuffleType=MPShuffleTypeItems;
        else cmdCenter.changeShuffleModeCommand.currentShuffleType=MPShuffleTypeOff;
                
        [self.detailViewController performSelectorOnMainThread:@selector(updMediaCenter) withObject:nil waitUntilDone:YES];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    if (self.detailViewController.mShuffle) cmdCenter.changeShuffleModeCommand.currentShuffleType=MPShuffleTypeItems;
    else cmdCenter.changeShuffleModeCommand.currentShuffleType=MPShuffleTypeOff;
    
    
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


    // Timer for updating playback progress without refreshing the UI
    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 1.0f target:self selector:@selector(updatePlaybackProgress) userInfo:nil repeats: YES];

    // Initialize Now Playing buttons
    [self refreshNowPlayingButtons];

    return TRUE;
}

///////////////////////
//Carplay
///////////////////////

- (void)playableContentManager:(MPPlayableContentManager *)contentManager didUpdateContext:(MPPlayableContentManagerContext *)context {
    //called when connecting/disconnecting
    if (context.endpointAvailable) {
        [self refreshMPItems];
        MPRemoteCommandCenter *cmdCenter=[MPRemoteCommandCenter sharedCommandCenter];
        [cmdCenter.likeCommand setEnabled:YES];
        [cmdCenter.dislikeCommand setEnabled:YES];
    } else {
        MPRemoteCommandCenter *cmdCenter=[MPRemoteCommandCenter sharedCommandCenter];
        [cmdCenter.likeCommand setEnabled:NO];
        [cmdCenter.dislikeCommand setEnabled:NO];
    }
}


/*/// Tells the data source to begin loading content items that are children of the
/// item specified by indexPath. This is provided so that applications can begin
/// asynchronous batched loading of content before MediaPlayer begins asking for
/// content items to display.
/// Client applications should always call the completion handler after loading
/// has finished, if this method is implemented.
- (void)beginLoadingChildItemsAtIndexPath:(NSIndexPath *)indexPath completionHandler:(void(^)(NSError * __nullable))completionHandler {
    if (completionHandler) completionHandler(nil);
}
*/
/// Tells MediaPlayer whether the content provided by the data source supports
/// playback progress as a property of its metadata.
/// If this method is not implemented, MediaPlayer will assume that progress is
/// not supported for any content items.
- (BOOL)childItemsDisplayPlaybackProgressAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section==0) return YES;
    else return NO;
}

/*
/// Provides a content item for the provided identifier.
/// Provide nil if there is no content item corresponding to the identifier.
/// Provide an error if there is an error that will not allow content items
/// to be retrieved.
/// Client applications should always call the completion handler after loading
/// has finished, if this method is implemented.
- (void)contentItemForIdentifier:(NSString *)identifier completionHandler:(void(^)(MPContentItem *__nullable, NSError * __nullable))completionHandler {
    
    if (completionHandler) completionHandler(nil,nil);
}
*/


/// Returns the number of child nodes at the specified index path. In a virtual
/// filesystem, this would be the number of files in a specific folder. An empty
/// index path represents the root node.
- (NSInteger)numberOfChildItemsAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.length) {
        return 0;
    }
    return [plArray count];
}

/// Returns the content item at the specified index path. If the content item is
/// mutated after returning, its updated contents will be sent to MediaPlayer.
- (nullable MPContentItem *)contentItemAtIndexPath:(NSIndexPath *)indexPath {
    MPContentItem *item=(MPContentItem*)([plArray objectAtIndex:indexPath.section]);
    return item;
}

/// This method is called when a media player interface wants to play a requested
/// content item. The application should call the completion handler with an
/// appropriate error if there was an error beginning playback for the item.

- (void)launchMostPlayedPL {
    NSMutableArray *arrayLabels=[[NSMutableArray alloc] init];
    NSMutableArray *arrayFullpaths=[[NSMutableArray alloc] init];
    int pl_entries;
    pl_entries=[rootVCLocalB loadMostPlayedList:arrayLabels fullpaths:arrayFullpaths];
    if (pl_entries) {
        t_playlist* playlist=(t_playlist*)calloc(1,sizeof(t_playlist));
                    
        playlist->playlist_name=[[NSString alloc] initWithFormat:@"Most played"];
        playlist->playlist_id=nil;
        playlist->nb_entries=pl_entries;
        for (int i=0;i<[arrayLabels count];i++) {
            playlist->entries[i].label=[arrayLabels objectAtIndex:i];
            playlist->entries[i].fullpath=[arrayFullpaths objectAtIndex:i];
            playlist->entries[i].ratings=-1;
        }
        
        [detailViewController play_listmodules:playlist start_index:0];
        [detailViewController playPushed];
        
        mdz_safe_free(playlist);
    }
}

- (void)launchFavoritesPL {
    NSMutableArray *arrayLabels=[[NSMutableArray alloc] init];
    NSMutableArray *arrayFullpaths=[[NSMutableArray alloc] init];
    int pl_entries;
    pl_entries=[rootVCLocalB loadFavoritesList:arrayLabels fullpaths:arrayFullpaths];
    if (pl_entries) {
        t_playlist* playlist=(t_playlist*)calloc(1,sizeof(t_playlist));
                    
        playlist->playlist_name=[[NSString alloc] initWithFormat:@"Favorites"];
        playlist->playlist_id=nil;
        playlist->nb_entries=pl_entries;
        for (int i=0;i<[arrayLabels count];i++) {
            playlist->entries[i].label=[arrayLabels objectAtIndex:i];
            playlist->entries[i].fullpath=[arrayFullpaths objectAtIndex:i];
            playlist->entries[i].ratings=-1;
        }
        
        [detailViewController play_listmodules:playlist start_index:0];
        [detailViewController playPushed];
        
        mdz_safe_free(playlist);
    }
}

- (void)launchRandomPL {
    NSMutableArray *arrayLabels=[[NSMutableArray alloc] init];
    NSMutableArray *arrayFullpaths=[[NSMutableArray alloc] init];
    int pl_entries;
    pl_entries=[rootVCLocalB loadLocalFilesRandomPL:arrayLabels fullpaths:arrayFullpaths];
    if (pl_entries) {
        t_playlist* playlist=(t_playlist*)calloc(1,sizeof(t_playlist));
                    
        playlist->playlist_name=[[NSString alloc] initWithFormat:@"Random"];
        playlist->playlist_id=nil;
        playlist->nb_entries=pl_entries;
        for (int i=0;i<[arrayLabels count];i++) {
            playlist->entries[i].label=[arrayLabels objectAtIndex:i];
            playlist->entries[i].fullpath=[arrayFullpaths objectAtIndex:i];
            playlist->entries[i].ratings=-1;
        }
        
        [detailViewController play_listmodules:playlist start_index:0];
        [detailViewController playPushed];
        
        mdz_safe_free(playlist);
    }
}

- (void)launchUserPL:(NSNumber*)pl_id {
    NSMutableArray *arrayLabels=[[NSMutableArray alloc] init];
    NSMutableArray *arrayFullpaths=[[NSMutableArray alloc] init];
    int pl_entries;
    pl_entries=[rootVCLocalB loadUserList:[pl_id intValue] labels:arrayLabels fullpaths:arrayFullpaths];
    if (pl_entries) {
        t_playlist* playlist=(t_playlist*)calloc(1,sizeof(t_playlist));
                    
        playlist->playlist_name=[[NSString alloc] initWithFormat:@"User playlist"];
        playlist->playlist_id=[NSString stringWithFormat:@"%d",[pl_id intValue]];
        playlist->nb_entries=pl_entries;
        for (int i=0;i<[arrayLabels count];i++) {
            playlist->entries[i].label=[arrayLabels objectAtIndex:i];
            playlist->entries[i].fullpath=[arrayFullpaths objectAtIndex:i];
            playlist->entries[i].ratings=-1;
        }
        
        [detailViewController play_listmodules:playlist start_index:0];
        [detailViewController playPushed];
        
        mdz_safe_free(playlist);
    }
}

- (void)playableContentManager:(MPPlayableContentManager *)contentManager initiatePlaybackOfContentItemAtIndexPath:(NSIndexPath *)indexPath completionHandler:(void(^)(NSError * __nullable))completionHandler {
    MPContentItem *item=(MPContentItem*)[plArray objectAtIndex:indexPath.section];
    if ([item.identifier isEqualToString:@"pl_NP"]) {
        //Now playing
        [self.detailViewController performSelectorOnMainThread:@selector(playPushed) withObject:nil waitUntilDone:YES];
    } else if ([item.identifier isEqualToString:@"pl_MP"]) {
        //Most played
        [self performSelectorOnMainThread:@selector(launchMostPlayedPL) withObject:nil waitUntilDone:YES];
    } else if ([item.identifier isEqualToString:@"pl_FV"]) {
        //Favorites
        [self performSelectorOnMainThread:@selector(launchFavoritesPL) withObject:nil waitUntilDone:YES];
    } else if ([item.identifier isEqualToString:@"pl_RD"]) {
        //Random
        [self performSelectorOnMainThread:@selector(launchRandomPL) withObject:nil waitUntilDone:YES];
    } else {
        //User playlist, id is pl_#xxx where xxx is the id
        const char *tmpstr=[item.identifier UTF8String];
        int id_pl=atoi(tmpstr+4);
        
        [self performSelectorOnMainThread:@selector(launchUserPL:) withObject:[NSNumber numberWithInt:id_pl] waitUntilDone:YES];
    }
    [self refreshMPItems];
    completionHandler(nil);
}


//////////////////////////


@end
