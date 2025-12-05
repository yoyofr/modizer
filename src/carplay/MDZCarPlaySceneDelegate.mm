//
//  MDZCarPlaySceneDelegate.m
//  modizer
//
//  Created by Yohann Magnien on 22/04/2021.
//

#import "MDZCarPlaySceneDelegate.h"

@class CarPlayAndRemoteManagement;

@interface MDZCarPlaySceneDelegate ()
@property (nonatomic, strong) CPInterfaceController *interfaceController;
@property (nonatomic, strong) CPListTemplate *playlistsTemplate;
@end

@implementation MDZCarPlaySceneDelegate

#pragma mark - CPTemplateApplicationSceneDelegate

- (void)templateApplicationScene:(CPTemplateApplicationScene *)templateApplicationScene
    didConnectInterfaceController:(CPInterfaceController *)interfaceController {

    NSLog(@"[CarPlay] didConnectInterfaceController called");

    self.interfaceController = interfaceController;

    // Connect with the CarPlayAndRemoteManagement instance
    [self connectToCarPlayManager];

    // Create the main playlists template
    self.playlistsTemplate = [self createPlaylistsTemplate];

    // Add "Now Playing" button that pushes the system Now Playing screen
    __weak typeof(self) weakSelf = self;
    CPBarButton *nowPlayingButton = [[CPBarButton alloc] initWithTitle:NSLocalizedString(@"Now Playing", @"")
                                                                handler:^(CPBarButton * _Nonnull button) {
        NSLog(@"[CarPlay] Now Playing button tapped");
        [weakSelf showNowPlayingScreen];
    }];

    self.playlistsTemplate.trailingNavigationBarButtons = @[nowPlayingButton];

    NSLog(@"[CarPlay] Setting playlists template with Now Playing button");

    // Set as root template
    [interfaceController setRootTemplate:self.playlistsTemplate animated:NO completion:^(BOOL success, NSError * _Nullable error) {
        if (success) {
            NSLog(@"[CarPlay] Template set successfully");
        } else {
            NSLog(@"[CarPlay] Error setting template: %@", error);
        }
    }];
}

- (void)connectToCarPlayManager {
    // Get the CarPlayAndRemoteManagement instance via MPPlayableContentManager
    MPPlayableContentManager *contentManager = [MPPlayableContentManager sharedContentManager];
    id<MPPlayableContentDelegate> delegate = contentManager.delegate;

    NSLog(@"[CarPlay] Connecting to CarPlayManager, delegate: %@", delegate);

    // Use NSClassFromString to avoid importing the header
    if ([delegate isKindOfClass:NSClassFromString(@"CarPlayAndRemoteManagement")]) {
        NSLog(@"[CarPlay] Found CarPlayAndRemoteManagement instance");
        // Use KVC to set the carPlaySceneDelegate property
        NSObject *carPlayManager = (NSObject *)delegate;
        [carPlayManager setValue:self forKey:@"carPlaySceneDelegate"];
        NSLog(@"[CarPlay] Connected successfully");
    } else {
        NSLog(@"[CarPlay] WARNING: delegate is not CarPlayAndRemoteManagement");
    }
}

- (void)templateApplicationScene:(CPTemplateApplicationScene *)templateApplicationScene
didDisconnectInterfaceController:(CPInterfaceController *)interfaceController {
    self.interfaceController = nil;
}

#pragma mark - Template Creation

- (CPListTemplate *)createPlaylistsTemplate {
    NSMutableArray *items = [NSMutableArray array];

    // Get the MPPlayableContentManager to access playlists
    MPPlayableContentManager *contentManager = [MPPlayableContentManager sharedContentManager];
    id<MPPlayableContentDataSource> dataSource = contentManager.dataSource;

    NSLog(@"[CarPlay] Creating playlists template, dataSource: %@", dataSource);

    if (dataSource) {
        // Get number of playlists - use empty indexPath for root level
        NSIndexPath *rootIndexPath = [[NSIndexPath alloc] init];
        NSInteger count = [dataSource numberOfChildItemsAtIndexPath:rootIndexPath];

        NSLog(@"[CarPlay] Found %ld playlists", (long)count);

        for (NSInteger i = 0; i < count; i++) {
            NSIndexPath *indexPath = [NSIndexPath indexPathWithIndex:i];
            MPContentItem *mpItem = [dataSource contentItemAtIndexPath:indexPath];

            NSLog(@"[CarPlay] Playlist %ld: %@", (long)i, mpItem.title);

            if (mpItem) {
                // Create CPListItem from MPContentItem
                CPListItem *item = [[CPListItem alloc] initWithText:mpItem.title
                                                         detailText:nil];

                // Set playback progress if available
                if (mpItem.playbackProgress > 0) {
                    NSString *detailText = [NSString stringWithFormat:@"Progress: %.0f%%",
                                          mpItem.playbackProgress * 100];
                    item = [[CPListItem alloc] initWithText:mpItem.title
                                                 detailText:detailText];
                }

                // Set handler to initiate playback
                __weak typeof(self) weakSelf = self;
                [item setHandler:^(id<CPSelectableListItem> _Nonnull selectableItem,
                                 dispatch_block_t _Nonnull completionBlock) {
                    [weakSelf playContentItemAtIndexPath:indexPath];
                    completionBlock();
                }];

                [items addObject:item];
            }
        }
    }

    // Create list template
    CPListTemplate *template = [[CPListTemplate alloc] initWithTitle:NSLocalizedString(@"Playlists",@"")
                                                             sections:@[
        [[CPListSection alloc] initWithItems:items]
    ]];

    return template;
}

- (void)showNowPlayingScreen {
    // Use the system's standard Now Playing screen
    CPNowPlayingTemplate *nowPlayingTemplate = [CPNowPlayingTemplate sharedTemplate];
    [self.interfaceController pushTemplate:nowPlayingTemplate animated:YES completion:nil];
}

- (void)playContentItemAtIndexPath:(NSIndexPath *)indexPath {
    // Use the MPPlayableContentManager delegate to initiate playback
    MPPlayableContentManager *contentManager = [MPPlayableContentManager sharedContentManager];
    id<MPPlayableContentDelegate> delegate = contentManager.delegate;

    if ([delegate respondsToSelector:@selector(playableContentManager:initiatePlaybackOfContentItemAtIndexPath:completionHandler:)]) {
        [delegate playableContentManager:contentManager
    initiatePlaybackOfContentItemAtIndexPath:indexPath
                       completionHandler:^(NSError * _Nullable error) {
            if (error) {
                NSLog(@"Error initiating playback: %@", error);
            } else {
                // Refresh the template to show updated "Now Playing" status
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)),
                             dispatch_get_main_queue(), ^{
                    [self refreshPlaylistsTemplate];
                });
            }
        }];
    }
}

- (void)refreshPlaylistsTemplate {
    if (!self.interfaceController) return;

    // If the playlists template is already the root, just update its sections to preserve scroll position
    if (self.playlistsTemplate && [self.interfaceController.rootTemplate isEqual:self.playlistsTemplate]) {
        // Create updated items
        NSMutableArray *items = [NSMutableArray array];

        MPPlayableContentManager *contentManager = [MPPlayableContentManager sharedContentManager];
        id<MPPlayableContentDataSource> dataSource = contentManager.dataSource;

        if (dataSource) {
            NSIndexPath *rootIndexPath = [[NSIndexPath alloc] init];
            NSInteger count = [dataSource numberOfChildItemsAtIndexPath:rootIndexPath];

            for (NSInteger i = 0; i < count; i++) {
                NSIndexPath *indexPath = [NSIndexPath indexPathWithIndex:i];
                MPContentItem *mpItem = [dataSource contentItemAtIndexPath:indexPath];

                if (mpItem) {
                    CPListItem *item = [[CPListItem alloc] initWithText:mpItem.title
                                                             detailText:nil];

                    if (mpItem.playbackProgress > 0) {
                        NSString *detailText = [NSString stringWithFormat:@"Progress: %.0f%%",
                                              mpItem.playbackProgress * 100];
                        item = [[CPListItem alloc] initWithText:mpItem.title
                                                     detailText:detailText];
                    }

                    __weak typeof(self) weakSelf = self;
                    [item setHandler:^(id<CPSelectableListItem> _Nonnull selectableItem,
                                     dispatch_block_t _Nonnull completionBlock) {
                        [weakSelf playContentItemAtIndexPath:indexPath];
                        completionBlock();
                    }];

                    [items addObject:item];
                }
            }
        }

        // Update sections without replacing the template (preserves scroll position)
        [self.playlistsTemplate updateSections:@[[[CPListSection alloc] initWithItems:items]]];
    } else {
        // Template not displayed yet, create and set it as root
        self.playlistsTemplate = [self createPlaylistsTemplate];

        __weak typeof(self) weakSelf = self;
        CPBarButton *nowPlayingButton = [[CPBarButton alloc] initWithTitle:NSLocalizedString(@"Now Playing", @"")
                                                                    handler:^(CPBarButton * _Nonnull button) {
            [weakSelf showNowPlayingScreen];
        }];
        self.playlistsTemplate.trailingNavigationBarButtons = @[nowPlayingButton];

        [self.interfaceController popToRootTemplateAnimated:NO completion:^(BOOL success, NSError * _Nullable error) {
            [self.interfaceController setRootTemplate:self.playlistsTemplate animated:YES completion:nil];
        }];
    }
}

#pragma mark - Public Methods (for external updates)

- (void)updatePlaylistsDisplay {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self refreshPlaylistsTemplate];
    });
}

@end
