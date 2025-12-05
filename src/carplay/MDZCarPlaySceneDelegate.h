//
//  MDZCarPlaySceneDelegate.h
//  modizer
//
//  Created by Yohann Magnien on 22/04/2021.
//

#import <UIKit/UIKit.h>
#import <CarPlay/CarPlay.h>
#import <MediaPlayer/MediaPlayer.h>

NS_ASSUME_NONNULL_BEGIN

@interface MDZCarPlaySceneDelegate : UIResponder <CPTemplateApplicationSceneDelegate>

// Method to update the playlists display when content changes
- (void)updatePlaylistsDisplay;

@end

NS_ASSUME_NONNULL_END
