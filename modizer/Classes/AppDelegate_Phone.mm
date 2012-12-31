//
//  AppDelegate_Phone.m
//  modizer4
//
//  Created by Yohann Magnien on 09/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

//#define GEN_EXT_LIST
#import "ModizerWin.h"
#import "AppDelegate_Phone.h"
#import "RootViewControllerIphone.h"
#import "DetailViewControllerIphone.h"

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include <sys/xattr.h>

static BOOL backgroundSupported;

char* strlower(char *Str);
extern "C" {

#include <pthread.h>
pthread_mutex_t uade_mutex;
}
pthread_mutex_t db_mutex;
pthread_mutex_t download_mutex;
pthread_mutex_t play_mutex;

@implementation AppDelegate_Phone

@synthesize modizerWin,tabBarController, rootViewControlleriPhone, detailViewControlleriPhone;


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

-(void)generateSupportExtList{
	NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
	NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
	NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
	NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
	NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
	NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
	NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_MODPLUG componentsSeparatedByString:@","];
	NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
	NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
	NSArray *filetype_extSEXYPSF=[SUPPORTED_FILETYPE_SEXYPSF componentsSeparatedByString:@","];
	NSArray *filetype_extAOSDK=[SUPPORTED_FILETYPE_AOSDK componentsSeparatedByString:@","];
	NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
	NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
	NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
			
	for (int i=0;i<[filetype_extMDX count];i++) {
		printf("%s\n",[[filetype_extMDX objectAtIndex:i] UTF8String]);
	}
	for (int i=0;i<[filetype_extSID count];i++) {
		printf("%s\n",[[filetype_extSID objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extSTSOUND count];i++) {
		printf("%s\n",[[filetype_extSTSOUND objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extSC68 count];i++) {
		printf("%s\n",[[filetype_extSC68 objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extARCHIVE count];i++) {
		printf("%s\n",[[filetype_extARCHIVE objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extUADE count];i++) {
		printf("%s\n",[[filetype_extUADE objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extMODPLUG count];i++) {
		printf("%s\n",[[filetype_extMODPLUG objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extGME count];i++) {
		printf("%s\n",[[filetype_extGME objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extADPLUG count];i++) {
		printf("%s\n",[[filetype_extADPLUG objectAtIndex:i] UTF8String]);
	}
	for (int i=0;i<[filetype_extSEXYPSF count];i++) {
		printf("%s\n",[[filetype_extSEXYPSF objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extAOSDK count];i++) {
		printf("%s\n",[[filetype_extAOSDK objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extHVL count];i++) {
		printf("%s\n",[[filetype_extHVL objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extSEXYPSF count];i++) {
		printf("%s\n",[[filetype_extSEXYPSF objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extAOSDK count];i++) {
		printf("%s\n",[[filetype_extAOSDK objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extHVL count];i++) {
		printf("%s\n",[[filetype_extHVL objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extGSF count];i++) {
		printf("%s\n",[[filetype_extGSF objectAtIndex:i] UTF8String] );
	}
	for (int i=0;i<[filetype_extASAP count];i++) {
		printf("%s\n",[[filetype_extASAP objectAtIndex:i] UTF8String] );
	}
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


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {    
	// Override point for customization after application launch
	//
    
	UIDevice* device = [UIDevice currentDevice];
	backgroundSupported = NO;
	if ([device respondsToSelector:@selector(isMultitaskingSupported)])
		backgroundSupported = device.multitaskingSupported;
	
#ifdef GEN_EXT_LIST	
	[self generateSupportExtList];
	return NO;
#else
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
	[[UIApplication sharedApplication] setStatusBarStyle: UIStatusBarStyleBlackOpaque];

    //battery: if charging, disable idleTimer
    [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];    
    [self batteryChanged:nil];
    
    modizerWin.rootViewController=tabBarController;
	[modizerWin addSubview:[tabBarController view]];
	[modizerWin makeKeyAndVisible];
	[rootViewControlleriPhone createEditableCopyOfDatabaseIfNeeded:FALSE quiet:0];   //Should be handled another way, for example on first DB access
    
    
		
	if ([[UIApplication sharedApplication] respondsToSelector:@selector(beginReceivingRemoteControlEvents)]) {
		[detailViewControlleriPhone enterBackground];
		[modizerWin becomeFirstResponder];
		[[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
	}	
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(batteryChanged:) name:@"UIDeviceBatteryLevelDidChangeNotification" object:[UIDevice currentDevice]];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(batteryChanged:) name:@"UIDeviceBatteryStateDidChangeNotification" object:[UIDevice currentDevice]];
    
    

    // Pasteboard notif
/*    [[NSNotificationCenter defaultCenter]
        addObserver:self
        selector:@selector(pasteboardChangedNotification:)
        name:UIPasteboardChangedNotification
        object:[UIPasteboard generalPasteboard]];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
        selector:@selector(pasteboardChangedNotification:)
        name:UIPasteboardRemovedNotification
        object:[UIPasteboard generalPasteboard]];*/
	//
    
    
    NSURL *url = (NSURL *)[launchOptions valueForKey:UIApplicationLaunchOptionsURLKey];
    if ([url isFileURL]) {
        NSString *filepath;
        filepath=[url path];
        NSRange r;
        r=[filepath rangeOfString:@"Documents/"];
        if (r.location!=NSNotFound) {
            NSString *shortfilepath=[filepath substringFromIndex:r.location];
            //if (detailViewControlleriPhone.sc_DefaultAction.selectedSegmentIndex==0)
            //[detailViewControlleriPhone add_to_playlist:shortfilepath fileName:[shortfilepath lastPathComponent]  forcenoplay:0];
            
            int pos=0;
            int total_entries=0;
            NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
            NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
            [array_label addObject:[shortfilepath lastPathComponent]];
            [array_path addObject:shortfilepath];
            total_entries=1;
            
            signed char *tmp_ratings;
            short int *tmp_playcounts;
            tmp_ratings=(signed char*)malloc(total_entries*sizeof(signed char));
            tmp_playcounts=(short int*)malloc(total_entries*sizeof(short int));
            tmp_ratings[0]=-1;
            tmp_playcounts[0]=0;
            
            [detailViewControlleriPhone play_listmodules:array_label start_index:pos path:array_path ratings:tmp_ratings playcounts:tmp_playcounts];
            
            free(tmp_ratings);
            free(tmp_playcounts);
        }
    }

    
	if (detailViewControlleriPhone.mPlaylist_size) {		
		//[detailViewControlleriPhone play_restart];  //Playlist not empty ; try to restart
        
	}
#endif
	
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
            //if (detailViewControlleriPhone.sc_DefaultAction.selectedSegmentIndex==0)
            //[detailViewControlleriPhone add_to_playlist:shortfilepath fileName:[shortfilepath lastPathComponent]  forcenoplay:0];
            
            int pos=0;
            int total_entries=0;
            NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
            NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
            [array_label addObject:[shortfilepath lastPathComponent]];
            [array_path addObject:shortfilepath];
            total_entries=1;
            
            signed char *tmp_ratings;
            short int *tmp_playcounts;
            tmp_ratings=(signed char*)malloc(total_entries*sizeof(signed char));
            tmp_playcounts=(short int*)malloc(total_entries*sizeof(short int));
            tmp_ratings[0]=-1;
            tmp_playcounts[0]=0;
            
            [detailViewControlleriPhone play_listmodules:array_label start_index:pos path:array_path ratings:tmp_ratings playcounts:tmp_playcounts];            
            
            free(tmp_ratings);
            free(tmp_playcounts);
        }
    }
}

- (void)applicationWillTerminate:(UIApplication *)application {
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
    [detailViewControlleriPhone saveSettings];
	[detailViewControlleriPhone updateFlagOnExit];
}

// iOS 4 background support
- (void)applicationDidEnterBackground:(UIApplication *)application {
	
	[detailViewControlleriPhone saveSettings];
	//if (backgroundSupported==NO) return;
	if (( (detailViewControlleriPhone.mPaused)&&(detailViewControlleriPhone.sc_bgPlay.selectedSegmentIndex==1) )||
		  (detailViewControlleriPhone.sc_bgPlay.selectedSegmentIndex==0) ) {
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
    [detailViewControlleriPhone saveSettings];
	//[super didReceiveMemoryWarning];
}

- (void)dealloc {
    [modizerWin release];
    [super dealloc];
}


@end
