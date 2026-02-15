//
//  AppDelegate_Phone.mm
//  modizer
//
//  Created by Yohann Magnien on 09/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

//#define GEN_EXT_LIST

#import <UserNotifications/UserNotifications.h>
#import "AppDelegate_Phone.h"
#import "SceneDelegate.h"
#import <MediaPlayer/MediaPlayer.h>

#import "RootViewControllerLocalBrowser.h"
#import "DetailViewControllerIphone.h"
#import "RootViewControllerPlaylist.h"
#import "SettingsGenViewController.h"
#import "CarPlayAndRemoteManagement.h"
#import "DownloadViewController.h"
#import "myTabBarController.h"

#import "ModizFileHelper.h"
#import "CloudStorageManager.h"

#import "ModizerTypes.h"


//GLOBAL VAR
void *ExtractProgressObserverContext = &ExtractProgressObserverContext;
void *ExtractBrowserListProgressObserverContext = &ExtractBrowserListProgressObserverContext;
void *LoadingProgressObserverContext = &LoadingProgressObserverContext;
extern bool mdz_macos_AOTplugin;
//

//#import <AVFoundation/AVFoundation.h>
//#import <AudioToolbox/AudioToolbox.h>
#include <sys/xattr.h>

char* strlower(char *Str);

static BOOL backgroundSupported;

char homedirectory[512*4];
char bundledirectory[512*4];
NSURL *icloudURL;
bool icloud_available;

extern volatile t_settings settings[MAX_SETTINGS];



extern "C" {

#include <pthread.h>
pthread_mutex_t uade_mutex;
}
pthread_mutex_t db_mutex;
pthread_mutex_t download_mutex;
pthread_mutex_t play_mutex;
pthread_mutex_t pm_mutex;
pthread_mutex_t gl_mutex;

@implementation AppDelegate_Phone

@synthesize tabBarC, rootViewControlleriPhone, detailViewControlleriPhone,playlistVC,downloadVC;
//@synthesize cpMngt;


- (BOOL)addSkipBackupAttributeToItemAtURL
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    const char* filePath = [documentsDirectory UTF8String];
    
    const char* attrName = "com.apple.MobileBackup";
    u_int8_t attrValue = 1;
    
    int result = setxattr(filePath, attrName, &attrValue, sizeof(attrValue), 0, 0);
    return result == 0;
}

-(void) batteryChanged:(NSNotification*)notification {
    if ([[UIDevice currentDevice] batteryState] != UIDeviceBatteryStateUnplugged)
        [UIApplication sharedApplication].idleTimerDisabled=YES;
    else [UIApplication sharedApplication].idleTimerDisabled=NO;
}

/*
*  System Versioning Preprocessor Macros
*/

-(void) getSupportedExtensionList {
    NSMutableArray *supportedExtension=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE_AND_DATAFILE];
    
    NSOrderedSet *orderedSet = [NSOrderedSet orderedSetWithArray:supportedExtension];
    NSArray *arrayWithoutDuplicates = [orderedSet array];
    
    NSSortDescriptor *sd = [[NSSortDescriptor alloc] initWithKey:nil ascending:YES];
    
    for (id object in [arrayWithoutDuplicates sortedArrayUsingDescriptors:@[sd]]) {
        printf("%s\n",[(NSString*)object UTF8String]);
    }
    
    //[supportedExtension release];
    supportedExtension=nil;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch
    //
    START_PROFILE
#ifdef GEN_EXT_LIST
    [self getSupportedExtensionList];
#endif
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    
    
    //check iCloud availability - initialize asynchronously via CloudStorageManager
    //Legacy variables are still set for backward compatibility
    icloud_available = false;
    icloudURL = nil;

    // Initialize cloud storage asynchronously on background thread
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [[CloudStorageManager sharedManager] initializeWithCompletion:^(BOOL success) {
            dispatch_async(dispatch_get_main_queue(), ^{
                // Update legacy variables for backward compatibility
                CloudStorageSource *nativeICloud = [[CloudStorageManager sharedManager] nativeICloudSource];
                if (nativeICloud && nativeICloud.isAccessible) {
                    icloud_available = true;
                    icloudURL = nativeICloud.resolvedURL;
                    MDZILog("CloudStorage: iCloud initialized at %@", icloudURL.path);
                } else {
                    icloud_available = false;
                    icloudURL = nil;
                    MDZILog("CloudStorage: iCloud not available");
                }

                // Post notification that cloud storage is ready
                [[NSNotificationCenter defaultCenter] postNotificationName:@"CloudStorageReady" object:nil];
            });
        }];
    });
    
    [mFileMngr createDirectoryAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:@"Documents/Downloads"] withIntermediateDirectories:true attributes:NULL error:NULL];
    
#ifdef DEBUG_MODIZER
    MDZILog("%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:@"Documents/Downloads"]);
#endif
    
    //create dir for projectm custom assets
    [mFileMngr createDirectoryAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents%s",PM_ROOT_FOLDER_CUSTOM]] withIntermediateDirectories:true attributes:NULL error:NULL];
    [mFileMngr createDirectoryAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents%s/presets",PM_ROOT_FOLDER_CUSTOM]] withIntermediateDirectories:true attributes:NULL error:NULL];
    [mFileMngr createDirectoryAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents%s/textures",PM_ROOT_FOLDER_CUSTOM]] withIntermediateDirectories:true attributes:NULL error:NULL];

    // REMOVED: Mac Catalyst window size code - now handled in SceneDelegate
    // The window setup is now handled in SceneDelegate's scene:willConnectToSession:
    
    snprintf(homedirectory,sizeof(homedirectory),"%s",[[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:@"modizer.app"] UTF8String]);
    
    snprintf(bundledirectory,sizeof(bundledirectory),"%s",[[[NSBundle mainBundle] bundlePath] UTF8String]);
    
    UIDevice* device = [UIDevice currentDevice];
    backgroundSupported = NO;
    if ([device respondsToSelector:@selector(isMultitaskingSupported)]) backgroundSupported = device.
        multitaskingSupported;
    
    
    if (pthread_mutex_init(&uade_mutex,NULL)) {
        printf("cannot create uade mutex");
        return NO;
    }
    if (pthread_mutex_init(&db_mutex,NULL)) {
        printf("cannot create db mutex");
        return NO;
    }
    if (pthread_mutex_init(&download_mutex,NULL)) {
        printf("cannot create download mutex");
        return NO;
    }
    if (pthread_mutex_init(&play_mutex,NULL)) {
        printf("cannot create play mutex");
        return NO;
    }
    if (pthread_mutex_init(&pm_mutex,NULL)) {
        printf("cannot create pm mutex");
        return NO;
    }
    if (pthread_mutex_init(&gl_mutex,NULL)) {
        printf("cannot create gl mutex");
        return NO;
    }
    //sqlite3_enable_shared_cache(1);
    

    //battery: if charging, disable idleTimer
    [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
    [self batteryChanged:nil];
    
    // REMOVED: All window setup code (lines 212-284 from original)
    // This is now handled in SceneDelegate's scene:willConnectToSession:
    // Including:
    // - modizerWin.rootViewController setup
    // - [modizerWin addSubview:...]
    // - [modizerWin makeKeyAndVisible]
    // - tabBarController badge setup
    // - remote control setup
    // - battery notifications
    // - CarPlay initialization
    // - AnimatedLaunchVC setup
    
    // Battery notifications are still needed here
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(batteryChanged:) name:@"UIDeviceBatteryLevelDidChangeNotification" object:[UIDevice currentDevice]];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(batteryChanged:) name:@"UIDeviceBatteryStateDidChangeNotification" object:[UIDevice currentDevice]];
    
    
    if (launchOptions) {
    /*NSURL *url = (NSURL *)[launchOptions valueForKey:UIApplicationLaunchOptionsURLKey];
        if (url) {
            if ([url isFileURL]) {
                NSString *filepath;
                filepath=[url path];
                NSRange r;
                r=[filepath rangeOfString:@"Documents/"];
                if (r.location!=NSNotFound) {
                    NSString *shortfilepath=[filepath substringFromIndex:r.location];
                    
                    t_playlist *pl;
                    pl=(t_playlist*)calloc(1,sizeof(t_playlist));
                    pl->nb_entries=1;
                    pl->entries[0].label=[shortfilepath lastPathComponent];
                    pl->entries[0].fullpath=shortfilepath;
                    pl->entries[0].ratings=-1;
                    pl->entries[0].playcounts=0;
                    [detailViewControlleriPhone play_listmodules:pl start_index:0];
                    free(pl);
                }
            }
        }*/
    }

    //[downloadVC refreshDownloadCountBadge];
    
    
    [[UILabel appearanceWhenContainedInInstancesOfClasses:@[[UIAlertController class]]] setNumberOfLines:2];
    [[UILabel appearanceWhenContainedInInstancesOfClasses:@[[UIAlertController class]]] setLineBreakMode:NSLineBreakByWordWrapping];//NSLineBreakByCharWrapping];
    //[[UILabel appearanceWhenContainedInInstancesOfClasses:@[[UIAlertController class]]] setFont:[UIFont systemFontOfSize:6.0]];
    
        return YES;
    
    END_PROFILE
    return YES;
}

- (BOOL)application:(UIApplication *)app openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey,id> *)options {
    
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
    return [self application:application openURL:url options:nil];
}


- (void)applicationWillTerminate:(UIApplication *)application {
    //MDZILog("applicationWillTerminate");

    // Stop music playback before termination to avoid "pure virtual function called"
    // crash when SID emulation objects are destroyed in wrong order during teardown
    [detailViewControlleriPhone stop];

    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    // Remove all delivered notifications
    [center removeAllDeliveredNotifications];
    // Remove all pending notifications
    [center removeAllPendingNotificationRequests];

    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    NSNumber *valNb;
    valNb=[[NSNumber alloc] initWithInt:0];
    [prefs setObject:valNb forKey:@"ModizerRunningForeGround"];
    [prefs synchronize];
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    
}

- (void)applicationWillResignActive:(UIApplication *)application {
    
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    
}

- (void)openFeature:(NSString *)feature {
    //MDZILog("open feature %@",feature);
}

- (BOOL)application:(UIApplication *)application
continueUserActivity:(NSUserActivity *)userActivity
 restorationHandler:(void (^)(NSArray<id<UIUserActivityRestoring>> * _Nullable))restorationHandler {
    
    if ([userActivity.activityType isEqualToString:@"com.yoyofr.modizer.openFeature"]) {
        NSString *feature = userActivity.userInfo[@"feature"];
        [self openFeature:feature];
        return YES;
    }
    return NO;
}

- (void)didReceiveMemoryWarning
{
    // default behavior is to release the view if it doesn't have a superview.
    
    // remember to clean up anything outside of this view's scope, such as
    // data cached in the class instance and other global data.
//    MDZELog("received a memory warning...");
    [SettingsGenViewController backupSettings];
    [detailViewControlleriPhone saveSettings];
    [downloadVC backupDownloadList];
    //[super didReceiveMemoryWarning];
}

- (void)dealloc {
    //[modizerWin release];
    //modizerWin=nil;
    //[super dealloc];
}

- (void)buildMenuWithBuilder:(id<UIMenuBuilder>)builder API_AVAILABLE(ios(13.0)) {
    if (builder.system != UIMenuSystem.mainSystem) {
        return;
    }
    
    [super buildMenuWithBuilder:builder];
#if TARGET_OS_MACCATALYST
    
    // Remove problematic auto-generated menus
    [builder removeMenuForIdentifier:UIMenuServices];
    [builder removeMenuForIdentifier:UIMenuSpelling];
    [builder removeMenuForIdentifier:UIMenuSubstitutions];
    [builder removeMenuForIdentifier:UIMenuTransformations];
    [builder removeMenuForIdentifier:UIMenuSpeech];
    
    // Supprimer des menus existants
    [builder removeMenuForIdentifier:UIMenuFormat];
    [builder removeMenuForIdentifier:UIMenuToolbar];
    [builder removeMenuForIdentifier:UIMenuEdit];
    
    // Ou supprimer des commandes spécifiques
    //[builder removeMenuForIdentifier:UIMenuPreferences];
    
    // Récupérer le menu View
        UIMenu *viewMenu = [builder menuForIdentifier:UIMenuView];
        
    if (viewMenu) {
        NSMutableArray *newChildren = [NSMutableArray array];
        
        for (UIMenuElement *element in viewMenu.children) {
            BOOL shouldKeep = YES;
            
            // Vérifier si c'est un UIMenu (sous-menu)
            if ([element isKindOfClass:[UIMenu class]]) {
                UIMenu *submenu = (UIMenu *)element;
                if ([submenu.identifier isEqual:UIMenuSidebar]) {
                    shouldKeep = NO;
                }
            }
            // Vérifier si c'est un UICommand (commande)
            //                else if ([element isKindOfClass:[UICommand class]]) {
            //                    UICommand *command = (UICommand *)element;
            //                    // Comparer par action ou title si pas d'identifier
            //                    if (command.action == @selector(toggleFullScreen:)) {
            //                        shouldKeep = NO;
            //                    }
            //                }
            
            if (shouldKeep) {
                [newChildren addObject:element];
            }
        }
#ifdef MDZ_MACOS_WINDOW_AOT
        if (mdz_macos_AOTplugin) {
            UIWindowScene *windowScene = (UIWindowScene *)[UIApplication.sharedApplication.connectedScenes.allObjects firstObject];
            SceneDelegate *sceneDelegate = (SceneDelegate*)windowScene.delegate;
            // Utiliser UIKeyCommand au lieu de UICommand pour pouvoir spécifier un input
            UIKeyCommand *floatCommand = [UIKeyCommand commandWithTitle:NSLocalizedString(@"Always on top",@"")
                                                                  image:nil
                                                                 action:@selector(toggleAlwaysOnTop)
                                                                  input:@"t"  // Minuscule
                                                          modifierFlags:UIKeyModifierCommand  // Pas de modificateurs
                                                           propertyList:nil];
            
            // Ajouter attributes pour Fn (fonction key)
            floatCommand.wantsPriorityOverSystemBehavior = YES;
            
            // État avec checkmark
            floatCommand.state = sceneDelegate.isWindowFloating ? UIMenuElementStateOn : UIMenuElementStateOff;
            
            if (sceneDelegate.isWindowFloating) [sceneDelegate enableAlwaysOnTop];
            else [sceneDelegate disableAlwaysOnTop];
            
            
            UIMenu *floatMenu = [UIMenu menuWithTitle:@""
                                                image:nil
                                           identifier:@"com.modizer.float"
                                              options:UIMenuOptionsDisplayInline
                                             children:@[floatCommand]];
            
            //[builder insertSiblingMenu:floatMenu afterMenuForIdentifier:UIMenuWindow];
            [newChildren addObject:floatMenu];
        }
#endif
        UIMenu *newViewMenu = [viewMenu menuByReplacingChildren:newChildren];
        [builder replaceMenuForIdentifier:UIMenuView withMenu:newViewMenu];
    }
    
    //
    
#endif
}

#ifdef MDZ_MACOS_WINDOW_AOT
#if TARGET_OS_MACCATALYST
-(void) toggleAlwaysOnTop {
    UIWindowScene *windowScene = (UIWindowScene *)[UIApplication.sharedApplication.connectedScenes.allObjects firstObject];
    SceneDelegate *sceneDelegate = (SceneDelegate*)windowScene.delegate;
    [sceneDelegate toggleAlwaysOnTop];
    
    // Rebuild menu
    [[UIMenuSystem mainSystem] setNeedsRebuild];
}
#endif
#endif

#pragma mark - UISceneSession Lifecycle

- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options {
    // Called when a new scene session is being created.
    // Use this method to select a configuration to create the new scene with.


    // Check if this is a CarPlay scene - try matching the role string
    if ([connectingSceneSession.role containsString:@"CarPlay"] ||
        [connectingSceneSession.role containsString:@"CPTemplate"]) {
        UISceneConfiguration *config = [[UISceneConfiguration alloc] initWithName:@"CarPlay Configuration" sessionRole:connectingSceneSession.role];
        config.delegateClass = NSClassFromString(@"MDZCarPlaySceneDelegate");
        config.sceneClass = NSClassFromString(@"CPTemplateApplicationScene");
        return config;
    }

    // Default configuration for main window scene
    UISceneConfiguration *config = [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
    config.delegateClass = [SceneDelegate class];
    return config;
}

- (void)application:(UIApplication *)application didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions {
    // Called when the user discards a scene session.
    // If any sessions were discarded while the application was not running, this will be called shortly after application:didFinishLaunchingWithOptions.
    // Use this method to release any resources that were specific to the discarded scenes, as they will not return.
}

#pragma mark - Menu / about
- (void)showAbout:(id)sender {
    // Afficher ta fenêtre About personnalisée
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"About Modizer",@"")
                                                                   message:
                                [NSString stringWithFormat:NSLocalizedString(@"Version %d.%d\n© Yohann Magnien / YoyoFR",@""),VERSION_MAJOR,VERSION_MINOR]
                                                            preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK"
                                              style:UIAlertActionStyleDefault
                                            handler:nil]];
    
    [self.window.rootViewController presentViewController:alert animated:YES completion:nil];
}

@end
