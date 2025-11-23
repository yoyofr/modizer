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
    if ([NSProcessInfo processInfo].isiOSAppOnMac) {
        windowScene.sizeRestrictions.minimumSize = CGSizeMake(MODIZER_MACM1_WIDTH_MIN, MODIZER_MACM1_HEIGHT_MIN);
        windowScene.sizeRestrictions.maximumSize = CGSizeMake(MODIZER_MACM1_WIDTH_MAX, MODIZER_MACM1_HEIGHT_MAX);
        windowScene.sizeRestrictions.allowsFullScreen = YES;
    }
    
    // With automatic storyboard loading, the window and root VC are created by UIKit.
    UIWindow *window = windowScene.windows.firstObject;
    if (!window) {
        // Fallback to keyWindow if needed
        window = [UIApplication sharedApplication].keyWindow;
    }
    
    for (UIOpenURLContext *ctx in connectionOptions.URLContexts) {
        NSURL *url = ctx.URL;
        // Route the URL to your content handler / player
        // e.g., [detailViewControlleriPhone openURL:url];
        
    }
}

- (void)sceneDidDisconnect:(UIScene *)scene {
    // Called as the scene is being released by the system.
}

- (void)sceneDidBecomeActive:(UIScene *)scene {
    // Called when the scene has moved from an inactive state to an active state.
}

- (void)sceneWillResignActive:(UIScene *)scene {
    // Called when the scene will move from an active state to an inactive state.
}

- (void)sceneWillEnterForeground:(UIScene *)scene {
    // Called as the scene transitions from the background to the foreground.
}

- (void)sceneDidEnterBackground:(UIScene *)scene {
    // Called as the scene transitions from the foreground to the background.
}

- (void)scene:(UIScene *)scene openURLContexts:(NSSet<UIOpenURLContext *> *)URLContexts {
    for (UIOpenURLContext *ctx in URLContexts) {
        NSURL *url = ctx.URL;
        [tabBarController openURL:url];
    }
}

@end
