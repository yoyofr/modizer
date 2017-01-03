//
//  AppDelegate_Phone.m
//  modizer4
//
//  Created by Yohann Magnien on 09/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

//#define GEN_EXT_LIST
#import "AppDelegate_Phone.h"

#import "ModizerWin.h"
#import "RootViewControllerLocalBrowser.h"
#import "DetailViewControllerIphone.h"
#import "RootViewControllerPlaylist.h"
#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include <sys/xattr.h>

char* strlower(char *Str);

static BOOL backgroundSupported;

char homedirectory[512];
char bundledirectory[512];


extern "C" {

#include <pthread.h>
pthread_mutex_t uade_mutex;
}
pthread_mutex_t db_mutex;
pthread_mutex_t download_mutex;
pthread_mutex_t play_mutex;
BOOL is_ios7,is_retina;

@implementation AppDelegate_Phone

@synthesize modizerWin,tabBarController, rootViewControlleriPhone, detailViewControlleriPhone,playlistVC;

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

-(int) isSlowDevice {
	return mSlowDevice;
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

//#include "ParserModland.hpp"

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	// Override point for customization after application launch
	//
    
    sprintf(homedirectory,"%s",[[NSHomeDirectory() stringByAppendingPathComponent:@"modizer.app"] UTF8String]);
    
    sprintf(bundledirectory,"%s",[[[NSBundle mainBundle] bundlePath] UTF8String]);
    
    char allmods_filepath[1024];
    sprintf(allmods_filepath,"%s/allmods.txt",bundledirectory);
    
    //parseModland(allmods_filepath);
    
	UIDevice* device = [UIDevice currentDevice];
	backgroundSupported = NO;
	if ([device respondsToSelector:@selector(isMultitaskingSupported)]) backgroundSupported = device.
        multitaskingSupported;
	
    if (SYSTEM_VERSION_LESS_THAN(@"7.0")) {
        is_ios7=FALSE;
    } else is_ios7=TRUE;

    if ([[UIScreen mainScreen] respondsToSelector:@selector(scale)] == YES && [[UIScreen mainScreen] scale] == 2.00) {
        // RETINA DISPLAY
        is_retina=TRUE;
    } else is_retina=FALSE;
    
    if (!is_ios7) {
        [[UIApplication sharedApplication] setStatusBarStyle: UIStatusBarStyleBlackOpaque];
    } else {
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    }
    
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
    
    modizerWin.rootViewController=tabBarController;
	[modizerWin addSubview:[tabBarController view]];
	[modizerWin makeKeyAndVisible];
	
    
//    playlistVC->browse_depth=0;
//    playlistVC->detailViewController=detailViewControlleriPhone;
    
    if ([[UIApplication sharedApplication] respondsToSelector:@selector(beginReceivingRemoteControlEvents)]) {
		[detailViewControlleriPhone enterBackground];
		[modizerWin becomeFirstResponder];
		[[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
	}	
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(batteryChanged:) name:@"UIDeviceBatteryLevelDidChangeNotification" object:[UIDevice currentDevice]];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(batteryChanged:) name:@"UIDeviceBatteryStateDidChangeNotification" object:[UIDevice currentDevice]];
        

    NSURL *url = (NSURL *)[launchOptions valueForKey:UIApplicationLaunchOptionsURLKey];
    if ([url isFileURL]) {
        NSString *filepath;
        filepath=[url path];
        NSRange r;
        r=[filepath rangeOfString:@"Documents/"];
        if (r.location!=NSNotFound) {
            NSString *shortfilepath=[filepath substringFromIndex:r.location];
            
            t_playlist pl;
            pl.nb_entries=1;
            pl.entries[0].label=[shortfilepath lastPathComponent];
            pl.entries[0].fullpath=shortfilepath;
            pl.entries[0].ratings=-1;
            pl.entries[0].playcounts=0;
            [detailViewControlleriPhone play_listmodules:&pl start_index:0];
        }
    }

    
	if (detailViewControlleriPhone.mPlaylist_size) {		
		//[detailViewControlleriPhone play_restart];  //Playlist not empty ; try to restart
        
	}
	
	return YES;
}
- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
    if ([url isFileURL]) {
        NSString *filepath;
        filepath=[url path];
        NSRange r;
        r=[filepath rangeOfString:@"Documents/"];
        if (r.location!=NSNotFound) {
            NSString *shortfilepath=[filepath substringFromIndex:r.location];
            t_playlist pl;
            pl.nb_entries=1;
            pl.entries[0].label=[shortfilepath lastPathComponent];
            pl.entries[0].fullpath=shortfilepath;
            pl.entries[0].ratings=-1;
            pl.entries[0].playcounts=0;
            [detailViewControlleriPhone play_listmodules:&pl start_index:0];
            return YES;
        }
    }
    return NO;
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
    [modizerWin release];
    [super dealloc];
}


@end
