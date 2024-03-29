//
//  AppDelegate_Phone.m
//  modizer4
//
//  Created by Yohann Magnien on 09/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

//#define GEN_EXT_LIST
#import "AppDelegate_Phone.h"
#import <MediaPlayer/MediaPlayer.h>



#import "ModizerWin.h"
#import "RootViewControllerLocalBrowser.h"
#import "DetailViewControllerIphone.h"
#import "RootViewControllerPlaylist.h"
#import "SettingsGenViewController.h"
#import "CarPlayAndRemoteManagement.h"

#import "ModizFileHelper.h"

extern volatile t_settings settings[MAX_SETTINGS];

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


extern "C" {

#include <pthread.h>
pthread_mutex_t uade_mutex;
}
pthread_mutex_t db_mutex;
pthread_mutex_t download_mutex;
pthread_mutex_t play_mutex;

@implementation AppDelegate_Phone

@synthesize modizerWin,tabBarController, rootViewControlleriPhone, detailViewControlleriPhone,playlistVC;
@synthesize cpMngt;


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

/*NSUInteger pasteboardChangeCount_;

- (void)pasteboardChangedNotification:(NSNotification*)notification {
    pasteboardChangeCount_ = [UIPasteboard generalPasteboard].changeCount;
    NSLog(@"pasteboard count : %d",pasteboardChangeCount_);
    UIPasteboard *genPB=[UIPasteboard generalPasteboard];
    NSArray *pbtypes=[genPB pasteboardTypes];
    for (int i=0;i<[pbtypes count];i++) {
        NSLog(@"got : %@",[pbtypes objectAtIndex:i]);
    }
}*/

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
#ifdef GEN_EXT_LIST
    [self getSupportedExtensionList];
#endif            
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    
    
    //check iCloud availability
    if (![mFileMngr ubiquityIdentityToken]) {
        icloud_available=false;
        icloudURL=nil;
        NSLog(@"iCloud not available");
    } else {
        icloud_available=true;
        NSLog(@"got iCloud token");
        NSURL *url=[mFileMngr URLForUbiquityContainerIdentifier:nil];
        
        if (url) {
            icloudURL=[url URLByAppendingPathComponent:@"Documents"];
            
            [mFileMngr createFileAtPath:[[icloudURL path] stringByAppendingPathComponent:@"put_files_here.modizer"] contents:NULL attributes:NULL];
        } else icloudURL=nil;
    }
    
    [mFileMngr createDirectoryAtPath:[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Downloads"] withIntermediateDirectories:true attributes:NULL error:NULL];
    
#ifdef DEBUG_MODIZER
    NSLog(@"%@",[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Downloads"]);
#endif
    
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"14.0"))
    if (@available(iOS 14.0, *)) {
        if ([NSProcessInfo processInfo].isiOSAppOnMac) {
            for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
                if ([scene isKindOfClass:[UIWindowScene class]]) {
                    UIWindowScene* windowScene = (UIWindowScene*) scene;
                    windowScene.sizeRestrictions.minimumSize = CGSizeMake(MODIZER_MACM1_WIDTH_MIN,MODIZER_MACM1_HEIGHT_MIN);
                    windowScene.sizeRestrictions.maximumSize = CGSizeMake(MODIZER_MACM1_WIDTH_MAX,MODIZER_MACM1_HEIGHT_MAX);
                }
            }
        }else{
            //not on macos
        }
    }
    
    //[SettingsGenViewController loadSettings];
    //[SettingsGenViewController restoreSettings];
 
    sprintf(homedirectory,"%s",[[NSHomeDirectory() stringByAppendingPathComponent:@"modizer.app"] UTF8String]);
    
    sprintf(bundledirectory,"%s",[[[NSBundle mainBundle] bundlePath] UTF8String]);
    
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
	//sqlite3_enable_shared_cache(1);
	

    //battery: if charging, disable idleTimer
    [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];    
    [self batteryChanged:nil];
    
    [rootViewControlleriPhone createEditableCopyOfDatabaseIfNeeded:FALSE quiet:0];
    
   if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"14.0"))
    if (@available(iOS 14.0, *)) {
        if ([NSProcessInfo processInfo].isiOSAppOnMac) {
            //CGRect frame = [modizerWin frame];
            //frame.size.height = MODIZER_MACM1_HEIGHT_MAX;
            //frame.size.width = MODIZER_MACM1_WIDTH_MAX;
            //[modizerWin setFrame: frame];
            //[modizerWin setBounds:frame];
        }
    }
    modizerWin.rootViewController=(UITabBarController*)tabBarController;
	[modizerWin addSubview:[(UITabBarController*)tabBarController view]];
	[modizerWin makeKeyAndVisible];
//    playlistVC->browse_depth=0;
//    playlistVC->detailViewController=detailViewControlleriPhone;
    
    if ([[UIApplication sharedApplication] respondsToSelector:@selector(beginReceivingRemoteControlEvents)]) {
		[detailViewControlleriPhone enterBackground];
		[modizerWin becomeFirstResponder];
		//[[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
	}	
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

	if (detailViewControlleriPhone.mPlaylist_size) {		
		//[detailViewControlleriPhone play_restart];  //Playlist not empty ; try to restart
	}
    
    cpMngt=[[CarPlayAndRemoteManagement alloc] init];
    cpMngt.detailViewController=detailViewControlleriPhone;
    cpMngt.rootVCLocalB=rootViewControlleriPhone;
    [cpMngt initCarPlayAndRemote];
    
    CGRect frame = [modizerWin frame];
    animatedLaunchVC=[[AnimatedLaunchVC alloc] initWithNibName:@"AnimatedLaunch" bundle:[NSBundle mainBundle]];
    animatedLaunchVC.view.frame=/*self.view.*/frame;
    animatedLaunchVC.localBrowserVC=rootViewControlleriPhone;
    
    [modizerWin addSubview:[animatedLaunchVC view]];
    
    //[self pushViewController:animatedLaunchVC animated:YES];
    [[UILabel appearanceWhenContainedInInstancesOfClasses:@[[UIAlertController class]]] setNumberOfLines:2];
    [[UILabel appearanceWhenContainedInInstancesOfClasses:@[[UIAlertController class]]] setLineBreakMode:NSLineBreakByCharWrapping];
    //[[UILabel appearanceWhenContainedInInstancesOfClasses:@[[UIAlertController class]]] setFont:[UIFont systemFontOfSize:6.0]];
    
    return YES;
}

- (BOOL)application:(UIApplication *)app openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey,id> *)options {
    if ([url isFileURL]) {
        NSString *filepath;
        filepath=[url path];
        
        NSString *imported_filepath;
        NSError *err;
        NSFileManager *mFileMngr=[[NSFileManager alloc] init];
        
        [mFileMngr createDirectoryAtPath:[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Downloads"] withIntermediateDirectories:true attributes:NULL error:NULL];
        
        imported_filepath=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Downloads"],[filepath lastPathComponent]];
        //////////////////
        ///Get access
        if ([url startAccessingSecurityScopedResource]) {
            ////////////////////
            //Download from icould if required
            
            NSNumber *isDownloadedValue = NULL;
            if ([mFileMngr isUbiquitousItemAtURL:url]) {
                BOOL success = [url getResourceValue:&isDownloadedValue forKey:NSURLUbiquitousItemIsDownloadedKey error:NULL];
                if (success && ![isDownloadedValue boolValue]) {
                    [[NSFileManager defaultManager] startDownloadingUbiquitousItemAtURL:url error:NULL];
    //                NSLog(@"file has to be downloaded");
                    
                    UIAlertView *alertDownloading = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"File is not available locally.\nTrigerring download from iCloud, please check in 'Files' application.",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil];
                    if (alertDownloading) [alertDownloading show];
                    
                    return YES;
                }
            }            
//            NSLog(@"URL secure access granted for %@\n",[url path]);
            
            if ([mFileMngr copyItemAtPath:filepath toPath:imported_filepath error:&err]) {
//                NSLog(@"file imported in 'Downloads' folder");
                [rootViewControlleriPhone refreshViewAfterDownload];
            } else {
//                NSLog(@"file not imported in 'Downloads' folder, error: %ld %@",err.code,[err localizedDescription]);
            }
            [url stopAccessingSecurityScopedResource];
        } else  {
//            NSLog(@"URL secure access refused for %@\n",[url path]);
        }
        
        NSString *shortfilepath=imported_filepath=[NSString stringWithFormat:@"Documents/Downloads/%@",[filepath lastPathComponent]];
//        NSLog(@"opening: %@",shortfilepath);
        //}
        t_playlist *pl;
        pl=(t_playlist*)calloc(1,sizeof(t_playlist));
        
        pl->nb_entries=1;
        pl->entries[0].label=[shortfilepath lastPathComponent];
        pl->entries[0].fullpath=shortfilepath;
        pl->entries[0].ratings=-1;
        pl->entries[0].playcounts=0;
        [detailViewControlleriPhone play_listmodules:pl start_index:0];
        free(pl);
        return YES;
        
    }
    return NO;
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
    return [self application:application openURL:url options:nil];
}

- (void)applicationWillTerminate:(UIApplication *)application {
    [SettingsGenViewController backupSettings];
	[detailViewControlleriPhone saveSettings];
	[detailViewControlleriPhone updateFlagOnExit];
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
/*    if (pasteboardChangeCount_ != [UIPasteboard generalPasteboard].changeCount) {
        [[NSNotificationCenter defaultCenter] 
         postNotificationName:UIPasteboardChangedNotification
         object:[UIPasteboard generalPasteboard]];
    }*/
    
	if ([[UIApplication sharedApplication] respondsToSelector:@selector(endReceivingRemoteControlEvents)]) {
	//	[[UIApplication sharedApplication] endReceivingRemoteControlEvents];
		[detailViewControlleriPhone enterForeground];
	}
    
    
}

- (void)applicationWillResignActive:(UIApplication *)application {
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
	if ([[UIApplication sharedApplication] respondsToSelector:@selector(beginReceivingRemoteControlEvents)]) {
		[detailViewControlleriPhone enterBackground];
	//	[modizerWin becomeFirstResponder];
	//	[[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
	}

    // Ensure that settings are saved if closed by OS after resigning active
    [SettingsGenViewController backupSettings];
    [detailViewControlleriPhone saveSettings];
	[detailViewControlleriPhone updateFlagOnExit];
}



// iOS 4 background support
- (void)applicationDidEnterBackground:(UIApplication *)application {
	[SettingsGenViewController backupSettings];
	[detailViewControlleriPhone saveSettings];
	//if (backgroundSupported==NO) return;
	if (( (detailViewControlleriPhone.mPaused)&&(settings[GLOB_BackgroundMode].detail.mdz_switch.switch_value==1) )||
		  (settings[GLOB_BackgroundMode].detail.mdz_switch.switch_value==0) ) {
		//exit app if not playing anything
        [detailViewControlleriPhone updateFlagOnExit];
		exit(0);
	}
}

- (void)didReceiveMemoryWarning
{ 
	// default behavior is to release the view if it doesn't have a superview.
	
	// remember to clean up anything outside of this view's scope, such as
	// data cached in the class instance and other global data.
	NSLog(@"received a memory warning...");
    [SettingsGenViewController backupSettings];
    [detailViewControlleriPhone saveSettings];
	//[super didReceiveMemoryWarning];
}

- (void)dealloc {
    //[modizerWin release];
    modizerWin=nil;
    //[super dealloc];
}


@end
