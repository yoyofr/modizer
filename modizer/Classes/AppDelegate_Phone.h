//
//  AppDelegate_Phone.h
//  modizer4
//
//  Created by Yohann Magnien on 09/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "ModizerConstants.h"

@class ModizerWin;
@class RootViewControllerIphone;
@class DetailViewControllerIphone;
@class RootViewControllerPlaylist;

@interface AppDelegate_Phone : NSObject <UIApplicationDelegate> {
	IBOutlet ModizerWin *modizerWin;
//    IBOutlet UIWindow *window;
	IBOutlet UITabBarController *tabBarController;
	
	IBOutlet RootViewControllerIphone *rootViewControlleriPhone;
	IBOutlet DetailViewControllerIphone *detailViewControlleriPhone;
    IBOutlet RootViewControllerPlaylist *playlistVC;
	
	UIBackgroundTaskIdentifier bgTask;
	int mSlowDevice;
}

//@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet ModizerWin *modizerWin;
@property (nonatomic, retain) IBOutlet RootViewControllerIphone *rootViewControlleriPhone;
@property (nonatomic, retain) IBOutlet RootViewControllerPlaylist *playlistVC;
@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewControlleriPhone;
@property (nonatomic, retain) IBOutlet UITabBarController *tabBarController;

-(int) isSlowDevice;
-(void) batteryChanged:(NSNotification*)notification;

@end

