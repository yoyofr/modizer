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

extern volatile t_settings settings[MAX_SETTINGS];

//#import <AVFoundation/AVFoundation.h>
//#import <AudioToolbox/AudioToolbox.h>
#include <sys/xattr.h>

char* strlower(char *Str);

static BOOL backgroundSupported;

char homedirectory[512*4];
char bundledirectory[512*4];


extern "C" {

#include <pthread.h>
pthread_mutex_t uade_mutex;
}
pthread_mutex_t db_mutex;
pthread_mutex_t download_mutex;
pthread_mutex_t play_mutex;
BOOL is_retina;

/*
@interface MyClass : NSObject
- (void)updateMainLoopObjC;
@end

@implementation MyClass
- (void)updateMainLoopObjC {
    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate date]];
}
extern "C" void updateMainLoopC(void) {
    @autoreleasepool {
        MyClass *obj = [[MyClass alloc] init];
        IMP methodIMP = [obj methodForSelector:@selector(updateMainLoopObjC)];
        void (*functionPointer)(id, SEL) = (void (*)(id, SEL))methodIMP;

        // Then call it:
        functionPointer(obj, @selector(updateMainLoopObjC));
    }
}
@end*/


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
    NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
    NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
    NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
    NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
    NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","];
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME_EXT componentsSeparatedByString:@","];
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extHC=[SUPPORTED_FILETYPE_HC_EXT componentsSeparatedByString:@","];
    NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF_EXT componentsSeparatedByString:@","];
    NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
    NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
    NSArray *filetype_ext2SF=[SUPPORTED_FILETYPE_2SF_EXT componentsSeparatedByString:@","];
    NSArray *filetype_extV2M=[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    
    NSMutableArray *supportedExtension=[[NSMutableArray alloc] init];
    [supportedExtension addObjectsFromArray:filetype_extSID];
    [supportedExtension addObjectsFromArray:filetype_extMDX];
    [supportedExtension addObjectsFromArray:filetype_extSTSOUND];
    [supportedExtension addObjectsFromArray:filetype_extSC68];
    [supportedExtension addObjectsFromArray:filetype_extUADE];
    [supportedExtension addObjectsFromArray:filetype_extMODPLUG];
    [supportedExtension addObjectsFromArray:filetype_extXMP];
    [supportedExtension addObjectsFromArray:filetype_extGME];
    [supportedExtension addObjectsFromArray:filetype_extADPLUG];
    [supportedExtension addObjectsFromArray:filetype_extHC];
    [supportedExtension addObjectsFromArray:filetype_extHVL];
    [supportedExtension addObjectsFromArray:filetype_extGSF];
    [supportedExtension addObjectsFromArray:filetype_extASAP];
    [supportedExtension addObjectsFromArray:filetype_extVGM];
    [supportedExtension addObjectsFromArray:filetype_extWMIDI];
    [supportedExtension addObjectsFromArray:filetype_extARCHIVE];
    [supportedExtension addObjectsFromArray:filetype_extPMD];
    [supportedExtension addObjectsFromArray:filetype_ext2SF];
    [supportedExtension addObjectsFromArray:filetype_extV2M];
    [supportedExtension addObjectsFromArray:filetype_extVGMSTREAM];
    
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
    
    [mFileMngr createDirectoryAtPath:[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Downloads"] withIntermediateDirectories:true attributes:NULL error:NULL];
    
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
    
    [SettingsGenViewController loadSettings];
    [SettingsGenViewController restoreSettings];
 
    
    sprintf(homedirectory,"%s",[[NSHomeDirectory() stringByAppendingPathComponent:@"modizer.app"] UTF8String]);
    
    sprintf(bundledirectory,"%s",[[[NSBundle mainBundle] bundlePath] UTF8String]);
    char allmods_filepath[1024];
    sprintf(allmods_filepath,"%s/allmods.txt",bundledirectory);
    
    //parseModland(allmods_filepath);
    
	UIDevice* device = [UIDevice currentDevice];
	backgroundSupported = NO;
	if ([device respondsToSelector:@selector(isMultitaskingSupported)]) backgroundSupported = device.
        multitaskingSupported;
	
    
    if ([[UIScreen mainScreen] respondsToSelector:@selector(scale)] == YES && [[UIScreen mainScreen] scale] == 2.00) {
        // RETINA DISPLAY
        is_retina=TRUE;
    } else is_retina=FALSE;
    
    /*if (!is_ios7) {
        [[UIApplication sharedApplication] setStatusBarStyle: UIStatusBarStyleBlackOpaque];
    } else {
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    }*/
    
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
            CGRect frame = [modizerWin frame];
            frame.size.height = 480*1.5f;
            frame.size.width = 480*1.5f;
            [modizerWin setFrame: frame];
            [modizerWin setBounds:frame];
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
	

	
	//return;
/***********************************************************************************/
/*	UIApplication*    app = [UIApplication sharedApplication];
	
    // Request permission to run in the background. Provide an
    // expiration handler in case the task runs long.
    NSAssert(bgTask == UIBackgroundTaskInvalid, nil);
	NSLog(@"yo2");
	
	bgTask = [app beginBackgroundTaskWithExpirationHandler:^{
        // Synchronize the cleanup call on the main thread in case
        // the task actually finishes at around the same time.
        dispatch_async(dispatch_get_main_queue(), ^{
            if (bgTask != UIBackgroundTaskInvalid)
            {
                [app endBackgroundTask:bgTask];
                self->bgTask = UIBackgroundTaskInvalid;
            }
        });
    }];
	
    // Start the long-running task and return immediately.
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        // Do the work associated with the task.
		while ( (exit_background==0) && (bgTask != UIBackgroundTaskInvalid)&&([app backgroundTimeRemaining] > 595.0f) ) {		
			NSLog(@"Remaining : %f, state : %d",[app backgroundTimeRemaining],[app applicationState]);
			[NSThread sleepForTimeInterval:1]; //1s
		}
		
        // Synchronize the cleanup call on the main thread in case
        // the expiration handler is fired at the same time.
        dispatch_async(dispatch_get_main_queue(), ^{
			if (bgTask != UIBackgroundTaskInvalid)
            {
				exit_background=1;
                [app endBackgroundTask:bgTask];
                self->bgTask = UIBackgroundTaskInvalid;
            }
        });
    });*/
/***********************************************************************************/
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
