//
//  AppDelegate_Phone.h
//  modizer
//
//  Created by Yohann Magnien on 09/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "ModizerConstants.h"
#import "AnimatedLaunchVC.h"

@class RootViewControllerLocalBrowser;
@class DetailViewControllerIphone;
@class RootViewControllerPlaylist;
@class myTabBarController;
@class CarPlayAndRemoteManagement;
@class AnimatedLaunchVC;
@class DownloadViewController;

@interface AppDelegate_Phone : UIResponder <UIApplicationDelegate> {
    AnimatedLaunchVC *animatedLaunchVC;
    
    RootViewControllerLocalBrowser *rootViewControlleriPhone;
    DetailViewControllerIphone *detailViewControlleriPhone;
    RootViewControllerPlaylist *playlistVC;
    DownloadViewController *downloadVC;
    myTabBarController *tabBarC;
    
    //CarPlayAndRemoteManagement *cpMngt;
    
    UIBackgroundTaskIdentifier bgTask;
}

@property (nonatomic, retain) RootViewControllerLocalBrowser *rootViewControlleriPhone;
@property (nonatomic, retain) RootViewControllerPlaylist *playlistVC;
@property (nonatomic, retain) DetailViewControllerIphone *detailViewControlleriPhone;
@property (nonatomic, retain) DownloadViewController *downloadVC;
@property (nonatomic, retain) myTabBarController *tabBarC;
//@property (nonatomic, retain) CarPlayAndRemoteManagement *cpMngt;

-(void) batteryChanged:(NSNotification*)notification;

@end
