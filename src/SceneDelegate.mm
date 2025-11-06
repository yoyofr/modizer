//
 //  SceneDelegate.m
 //  modizer
 //
 //  Created by Yohann Magnien
 //  Copyright __YoyoFR / Yohann Magnien__. All rights reserved.
 //

#import "SceneDelegate.h"
#import "AppDelegate_Phone.h"
#import "ModizerWin.h"
#import "myTabBarController.h"
#import "RootViewControllerLocalBrowser.h"
#import "DetailViewControllerIphone.h"
#import "RootViewControllerPlaylist.h"
#import "DownloadViewController.h"
#import "AnimatedLaunchVC.h"
#import "CarPlayAndRemoteManagement.h"
#import "ModizerConstants.h"
#import "ModizerTypes.h"

@implementation SceneDelegate

@synthesize modizerWin,tabBarController, rootViewControlleriPhone, detailViewControlleriPhone,playlistVC,downloadVC;

#pragma mark - Helpers

- (id)findChildOfClass:(Class)cls inTabBarController:(UITabBarController *)tbc {
    for (UIViewController *vc in tbc.viewControllers) {
        // Unwrap nav controllers if present
        UIViewController *candidate = vc;
        if ([vc isKindOfClass:[UINavigationController class]]) {
            candidate = ((UINavigationController *)vc).viewControllers.firstObject;
        }
        if ([candidate isKindOfClass:cls]) {
            return candidate;
        }
    }
    return nil;
}

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    
    if (![scene isKindOfClass:[UIWindowScene class]]) {
        return;
    }
    
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    
    // Set window size restrictions for Mac Catalyst
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"14.0")) {
        if (@available(iOS 14.0, *)) {
            if ([NSProcessInfo processInfo].isiOSAppOnMac) {
                windowScene.sizeRestrictions.minimumSize = CGSizeMake(MODIZER_MACM1_WIDTH_MIN, MODIZER_MACM1_HEIGHT_MIN);
                windowScene.sizeRestrictions.maximumSize = CGSizeMake(MODIZER_MACM1_WIDTH_MAX, MODIZER_MACM1_HEIGHT_MAX);
            }
        }
    }

    // With automatic storyboard loading, the window and root VC are created by UIKit.
    UIWindow *window = windowScene.windows.firstObject;
    if (!window) {
        // Fallback to keyWindow if needed
        window = [UIApplication sharedApplication].keyWindow;
    }
    
    //[self pushViewController:animatedLaunchVC animated:YES];
}

- (void)sceneDidDisconnect:(UIScene *)scene {
    // Called as the scene is being released by the system.
    [SettingsGenViewController backupSettings];
    [detailViewControlleriPhone saveSettings];
    [downloadVC backupDownloadList];
    [detailViewControlleriPhone updateFlagOnExit];
    
}

- (void)sceneDidBecomeActive:(UIScene *)scene {
    // Called when the scene has moved from an inactive state to an active state.
    if ([[UIApplication sharedApplication] respondsToSelector:@selector(endReceivingRemoteControlEvents)]) {
    //    [[UIApplication sharedApplication] endReceivingRemoteControlEvents];
        [detailViewControlleriPhone enterForeground];
    }
}

- (void)sceneWillResignActive:(UIScene *)scene {
    // Called when the scene will move from an active state to an inactive state.
    if ([[UIApplication sharedApplication] respondsToSelector:@selector(beginReceivingRemoteControlEvents)]) {
        [detailViewControlleriPhone enterBackground];
    //    [modizerWin becomeFirstResponder];
    //    [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
    }
}

- (void)sceneWillEnterForeground:(UIScene *)scene {
    // Called as the scene transitions from the background to the foreground.
    //AppDelegate_Phone *appDelegate = (AppDelegate_Phone *)[UIApplication sharedApplication].delegate;
    if (downloadVC) {
        [downloadVC refreshDownloadCountBadge];
    }
}

- (void)sceneDidEnterBackground:(UIScene *)scene {
    // Called as the scene transitions from the foreground to the background.
    [downloadVC refreshDownloadCountBadge];
}

@end
