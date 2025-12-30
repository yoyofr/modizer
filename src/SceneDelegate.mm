//
 //  SceneDelegate.mm
 //  modizer
 //
 //  Created by Yohann Magnien
 //  Copyright __YoyoFR / Yohann Magnien__. All rights reserved.
 //

#import "SceneDelegate.h"
#import "AppDelegate_Phone.h"
#import "myTabBarController.h"
#import "RootViewControllerLocalBrowser.h"
#import "DetailViewControllerIphone.h"
#import "RootViewControllerPlaylist.h"
#import "DownloadViewController.h"
#import "AnimatedLaunchVC.h"
#import "CarPlayAndRemoteManagement.h"
#import "ModizerConstants.h"
#import "ModizerTypes.h"

bool mdz_macos_AOTplugin=false;

@implementation SceneDelegate

@synthesize tabBarController, rootViewControlleriPhone, detailViewControlleriPhone,playlistVC,downloadVC;

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
        if (@available(macCatalyst 16.0, *)) {
            windowScene.sizeRestrictions.allowsFullScreen = YES;
        } else {
            // Fallback on earlier versions
        }
    }
#if TARGET_OS_MACCATALYST
    windowScene.sizeRestrictions.minimumSize = CGSizeMake(MODIZER_MACM1_WIDTH_MIN, MODIZER_MACM1_HEIGHT_MIN);
    windowScene.sizeRestrictions.maximumSize = CGSizeMake(MODIZER_MACM1_WIDTH_MAX, MODIZER_MACM1_HEIGHT_MAX);
    if (@available(macCatalyst 16.0, *)) {
        windowScene.sizeRestrictions.allowsFullScreen = YES;
    } else {
        // Fallback on earlier versions
    }
#endif
    
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
    
#ifdef MDZ_MACOS_WINDOW_AOT
#if TARGET_OS_MACCATALYST
    // Restaurer l'état sauvegardé
        self.isWindowFloating = [[NSUserDefaults standardUserDefaults] boolForKey:@"WindowFloating"];
    
    [self loadMacPlugin];
    
    // Appliquer l'état au démarrage
    if (self.isWindowFloating) {
        Class macWindowManager = NSClassFromString(@"ModizerMacWindowManager");
        if (macWindowManager) {
            [macWindowManager performSelector:NSSelectorFromString(@"enableAlwaysOnTop")];
        }
    }
#endif
#endif
}

#ifdef MDZ_MACOS_WINDOW_AOT
#if TARGET_OS_MACCATALYST
- (void)loadMacPlugin {
    NSBundle *mainBundle = [NSBundle mainBundle];
    
    // Sur Mac Catalyst, le chemin est différent
    NSString *pluginPath = nil;
    
    // Méthode 1 : Chercher dans Contents/PlugIns
    NSString *pluginsDir = [[mainBundle bundlePath] stringByAppendingPathComponent:@"Contents/PlugIns"];
    pluginPath = [pluginsDir stringByAppendingPathComponent:@"ModizerMacWindowPlugin.bundle"];
    
    //(@"Looking for plugin at: %@", pluginPath);
    
    if (![[NSFileManager defaultManager] fileExistsAtPath:pluginPath]) {
        NSLog(@"❌ Plugin not found at: %@", pluginPath);
        
        // Méthode 2 : Chercher avec builtInPlugInsPath
        NSURL *pluginsURL = [mainBundle builtInPlugInsURL];
        if (pluginsURL) {
            pluginPath = [[pluginsURL path] stringByAppendingPathComponent:@"ModizerMacWindowPlugin.bundle"];
            NSLog(@"Trying builtInPlugInsURL: %@", pluginPath);
        }
    }
    
    if ([[NSFileManager defaultManager] fileExistsAtPath:pluginPath]) {
        //NSLog(@"✅ Plugin found at: %@", pluginPath);
        
        NSBundle *pluginBundle = [NSBundle bundleWithPath:pluginPath];
        
        if (pluginBundle) {
            NSError *error = nil;
            if ([pluginBundle loadAndReturnError:&error]) {
                //NSLog(@"✅ Plugin loaded successfully");
                
                // Vérifier que la classe existe
                Class macWindowManager = NSClassFromString(@"ModizerMacWindowManager");
                if (macWindowManager) {
//                    NSLog(@"✅ ModizerMacWindowManager class found!");
                    mdz_macos_AOTplugin=true;
                    // Rebuild menu
                    [[UIMenuSystem mainSystem] setNeedsRebuild];
                } else {
                    NSLog(@"❌ ModizerMacWindowManager class not found");
                }
            } else {
                NSLog(@"❌ Failed to load plugin: %@", error);
            }
        } else {
            NSLog(@"❌ Could not create bundle from path");
        }
    } else {
        NSLog(@"❌ Plugin file does not exist at: %@", pluginPath);
        
        // Debug : lister ce qui est dans PlugIns
//        NSArray *contents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:pluginsDir error:nil];
//        NSLog(@"Contents of PlugIns directory: %@", contents);
    }
}
- (void)toggleAlwaysOnTop {
    self.isWindowFloating = !self.isWindowFloating;
    
    // Sauvegarder l'état
        [[NSUserDefaults standardUserDefaults] setBool:self.isWindowFloating forKey:@"WindowFloating"];
        
    
    Class macWindowManager = NSClassFromString(@"ModizerMacWindowManager");
    if (macWindowManager) {
        SEL selector = NSSelectorFromString(@"setAlwaysOnTop:");
        
        if ([macWindowManager respondsToSelector:selector]) {
            // Utiliser NSInvocation pour passer un BOOL
            NSMethodSignature *signature = [macWindowManager methodSignatureForSelector:selector];
            NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
            [invocation setTarget:macWindowManager];
            [invocation setSelector:selector];
            [invocation setArgument:&_isWindowFloating atIndex:2];
            [invocation invoke];
            
            //NSLog(@"Window floating: %@", self.isWindowFloating ? @"ON" : @"OFF");
        }
    } else {
        NSLog(@"❌ ModizerMacWindowManager class not found");
    }
}

- (void)enableAlwaysOnTop {
    Class macWindowManager = NSClassFromString(@"ModizerMacWindowManager");
    if (macWindowManager) {
        [macWindowManager performSelector:NSSelectorFromString(@"enableAlwaysOnTop")];
        self.isWindowFloating = YES;
    }
}

- (void)disableAlwaysOnTop {
    Class macWindowManager = NSClassFromString(@"ModizerMacWindowManager");
    if (macWindowManager) {
        [macWindowManager performSelector:NSSelectorFromString(@"disableAlwaysOnTop")];
        self.isWindowFloating = NO;
    }
}
#endif
#endif

- (void)sceneDidDisconnect:(UIScene *)scene {
    // Called as the scene is being released by the system.
}

- (void)sceneDidBecomeActive:(UIScene *)scene {
    // Called when the scene has moved from an inactive state to an active state.
    //if ([[UIApplication sharedApplication] respondsToSelector:@selector(endReceivingRemoteControlEvents)]) {
    //    [[UIApplication sharedApplication] endReceivingRemoteControlEvents];
        [detailViewControlleriPhone enterForeground];
    //}

    [downloadVC restoreDownloadList];

    // Refresh CarPlay when app becomes active
    if (tabBarController && tabBarController.cpMngt) {
        [tabBarController.cpMngt refreshMPItems];
    }
}

- (void)sceneWillResignActive:(UIScene *)scene {
    // Called when the scene will move from an active state to an inactive state.
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
    //if ([[UIApplication sharedApplication] respondsToSelector:@selector(beginReceivingRemoteControlEvents)]) {
    [detailViewControlleriPhone enterBackground];
//        [tabBarC becomeFirstResponder];
//        [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
    //}

    // Ensure that settings are saved if closed by OS after resigning active
    [SettingsGenViewController backupSettings];
    [detailViewControlleriPhone saveSettings];
    [downloadVC backupDownloadList];
}

- (void)sceneWillEnterForeground:(UIScene *)scene {
    // Called as the scene transitions from the background to the foreground.
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    NSNumber *valNb;
    valNb=[[NSNumber alloc] initWithInt:1];
    [prefs setObject:valNb forKey:@"ModizerRunningForeGround"];
    [prefs synchronize];
    
    [downloadVC refreshDownloadCountBadge];
}

- (void)cleanupTempData {
    NSArray* temp = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:NSTemporaryDirectory() error:NULL];
    for (NSString *file in temp) {
        NSString *str=(NSString *)file;
        if ([str containsString:@"MDZNotif"]) [[NSFileManager defaultManager] removeItemAtPath:[NSString stringWithFormat:@"%@%@", NSTemporaryDirectory(), file] error:NULL];
    }
}

- (void)removeAllNotifications {
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    
    // Remove all delivered notifications
    [center removeAllDeliveredNotifications];
    
    // Remove all pending notifications
    [center removeAllPendingNotificationRequests];
    
    //MDZDLog("All notifications removed");
}



- (void)sceneDidEnterBackground:(UIScene *)scene {
    // Called as the scene transitions from the foreground to the background.
    [SettingsGenViewController backupSettings];
    [detailViewControlleriPhone saveSettings];
    [downloadVC backupDownloadList];
    
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    NSNumber *valNb;
    valNb=[[NSNumber alloc] initWithInt:0];
    [prefs setObject:valNb forKey:@"ModizerRunningForeGround"];
    [prefs synchronize];
    
    [self cleanupTempData];
}

- (void)scene:(UIScene *)scene openURLContexts:(NSSet<UIOpenURLContext *> *)URLContexts {
    for (UIOpenURLContext *ctx in URLContexts) {
        NSURL *url = ctx.URL;
        [tabBarController openURL:url];
    }
}


@end
