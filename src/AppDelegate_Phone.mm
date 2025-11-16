//
//  AppDelegate_Phone.m
//  modizer4
//
//  Created by Yohann Magnien on 09/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

//#define GEN_EXT_LIST

#import <UserNotifications/UserNotifications.h>
#import "AppDelegate_Phone.h"
#import "SceneDelegate.h"
#import <MediaPlayer/MediaPlayer.h>

#import "ModizerWin.h"
#import "RootViewControllerLocalBrowser.h"
#import "DetailViewControllerIphone.h"
#import "RootViewControllerPlaylist.h"
#import "SettingsGenViewController.h"
#import "CarPlayAndRemoteManagement.h"
#import "DownloadViewController.h"
#import "myTabBarController.h"

#import "ModizFileHelper.h"

#import "ModizerTypes.h"


//GLOBAL VAR
void *ExtractProgressObserverContext = &ExtractProgressObserverContext;
void *LoadingProgressObserverContext = &LoadingProgressObserverContext;
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

- (void)cleanupTempData {
    NSArray* temp = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:NSTemporaryDirectory() error:NULL];
    for (NSString *file in temp) {
        NSString *str=(NSString *)file;
        if ([str containsString:@"MDZNotif"]) [[NSFileManager defaultManager] removeItemAtPath:[NSString stringWithFormat:@"%@%@", NSTemporaryDirectory(), file] error:NULL];
    }
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch
    //
    START_PROFILE
#ifdef GEN_EXT_LIST
    [self getSupportedExtensionList];
#endif
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    
    
    //check iCloud availability
    if (![mFileMngr ubiquityIdentityToken]) {
        icloud_available=false;
        icloudURL=nil;
        MDZILog("iCloud not available");
    } else {
        icloud_available=true;
        NSURL *url=[mFileMngr URLForUbiquityContainerIdentifier:nil];
        
        if (url) {
            icloudURL=[url URLByAppendingPathComponent:@"Documents"];
            
            [mFileMngr createFileAtPath:[[icloudURL path] stringByAppendingPathComponent:@"put_files_here.modizer"] contents:NULL attributes:NULL];
        } else icloudURL=nil;
    }
    
    [mFileMngr createDirectoryAtPath:[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Downloads"] withIntermediateDirectories:true attributes:NULL error:NULL];
    
#ifdef DEBUG_MODIZER
    MDZILog("%@",[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Downloads"]);
#endif
    
    //create dir for projectm custom assets
    [mFileMngr createDirectoryAtPath:[NSHomeDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents%s",PM_ROOT_FOLDER_CUSTOM]] withIntermediateDirectories:true attributes:NULL error:NULL];
    [mFileMngr createDirectoryAtPath:[NSHomeDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents%s/presets",PM_ROOT_FOLDER_CUSTOM]] withIntermediateDirectories:true attributes:NULL error:NULL];
    [mFileMngr createDirectoryAtPath:[NSHomeDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents%s/textures",PM_ROOT_FOLDER_CUSTOM]] withIntermediateDirectories:true attributes:NULL error:NULL];

    //create dir for shaders cache
    [mFileMngr createDirectoryAtPath:[NSHomeDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents%s",SHADER_CACHE_DIR]] withIntermediateDirectories:true attributes:NULL error:NULL];
        
    
    // REMOVED: Mac Catalyst window size code - now handled in SceneDelegate
    // The window setup is now handled in SceneDelegate's scene:willConnectToSession:
    
    snprintf(homedirectory,sizeof(homedirectory),"%s",[[NSHomeDirectory() stringByAppendingPathComponent:@"modizer.app"] UTF8String]);
    
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
        printf("cannot create gl mutex");
        return NO;
    }
    //sqlite3_enable_shared_cache(1);
    

    //battery: if charging, disable idleTimer
    [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
    [self batteryChanged:nil];
    
    //[rootViewControlleriPhone createEditableCopyOfDatabaseIfNeeded:FALSE quiet:0];
    
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
    
    
    END_PROFILE
    return YES;
}

- (BOOL)application:(UIApplication *)app openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey,id> *)options {
    
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
    return [self application:application openURL:url options:nil];
}

- (void)removeAllNotifications {
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    
    // Remove all delivered notifications
    [center removeAllDeliveredNotifications];
    
    // Remove all pending notifications
    [center removeAllPendingNotificationRequests];
    
    //MDZDLog("All notifications removed");
}

- (void)applicationWillTerminate:(UIApplication *)application {
    [SettingsGenViewController backupSettings];
    [detailViewControlleriPhone saveSettings];
    [downloadVC backupDownloadList];
    [detailViewControlleriPhone updateFlagOnExit];
    
    [self removeAllNotifications];
    [self cleanupTempData];
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    [downloadVC refreshDownloadCountBadge];
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    if ([[UIApplication sharedApplication] respondsToSelector:@selector(endReceivingRemoteControlEvents)]) {
    //    [[UIApplication sharedApplication] endReceivingRemoteControlEvents];
        [detailViewControlleriPhone enterForeground];
    }
    
    [downloadVC restoreDownloadList];
}

- (void)applicationWillResignActive:(UIApplication *)application {
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
    if ([[UIApplication sharedApplication] respondsToSelector:@selector(beginReceivingRemoteControlEvents)]) {
        [detailViewControlleriPhone enterBackground];
//        [tabBarC becomeFirstResponder];
//        [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
    }

    // Ensure that settings are saved if closed by OS after resigning active
//    [SettingsGenViewController backupSettings];
//    [detailViewControlleriPhone saveSettings];
//    [downloadVC backupDownloadList];
//    [detailViewControlleriPhone updateFlagOnExit];
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
//    [SettingsGenViewController backupSettings];
//    [detailViewControlleriPhone saveSettings];
}

- (void)openFeature:(NSString *)feature {
    MDZILog("open feature %@",feature);
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
//    [downloadVC backupDownloadList];
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
    
    // Remove problematic auto-generated menus
    [builder removeMenuForIdentifier:UIMenuServices];
    [builder removeMenuForIdentifier:UIMenuSpelling];
    [builder removeMenuForIdentifier:UIMenuSubstitutions];
    [builder removeMenuForIdentifier:UIMenuTransformations];
    [builder removeMenuForIdentifier:UIMenuSpeech];
}

#pragma mark - UISceneSession Lifecycle

- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options {
    // Called when a new scene session is being created.
    // Use this method to select a configuration to create the new scene with.
    UISceneConfiguration *config = [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
    config.delegateClass = [SceneDelegate class];
    return config;
}

- (void)application:(UIApplication *)application didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions {
    // Called when the user discards a scene session.
    // If any sessions were discarded while the application was not running, this will be called shortly after application:didFinishLaunchingWithOptions.
    // Use this method to release any resources that were specific to the discarded scenes, as they will not return.
}

@end
