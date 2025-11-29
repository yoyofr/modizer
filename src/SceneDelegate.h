//
//  SceneDelegate.h
//  modizer4
//
//  Created by Yohann Magnien
//  Copyright __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <UIKit/UIKit.h>

@class RootViewControllerLocalBrowser;
@class DetailViewControllerIphone;
@class RootViewControllerPlaylist;
@class myTabBarController;
@class CarPlayAndRemoteManagement;
@class AnimatedLaunchVC;
@class DownloadViewController;

@interface SceneDelegate : UIResponder <UIWindowSceneDelegate> {
    UIWindow *window;
         myTabBarController *tabBarController;
         RootViewControllerLocalBrowser *rootViewControlleriPhone;
         DetailViewControllerIphone *detailViewControlleriPhone;
         RootViewControllerPlaylist *playlistVC;
         DownloadViewController *downloadVC;

}

@property (strong, nonatomic) UIWindow *window;
@property (nonatomic, retain)  RootViewControllerLocalBrowser *rootViewControlleriPhone;
@property (nonatomic, retain)  RootViewControllerPlaylist *playlistVC;
@property (nonatomic, retain)  DetailViewControllerIphone *detailViewControlleriPhone;
@property (nonatomic, retain)  myTabBarController *tabBarController;
@property (nonatomic, retain)  DownloadViewController *downloadVC;

@property (nonatomic, retain) AnimatedLaunchVC *animatedLaunchVC;
@property (nonatomic, retain) CarPlayAndRemoteManagement *cpMngt;

@end
