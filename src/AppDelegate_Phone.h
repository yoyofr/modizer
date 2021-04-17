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
@class RootViewControllerLocalBrowser;
@class DetailViewControllerIphone;
@class RootViewControllerPlaylist;
@class myTabBarController;

@interface AppDelegate_Phone : UIResponder <UIApplicationDelegate> {
	IBOutlet ModizerWin *modizerWin;
//    IBOutlet UIWindow *window;
	IBOutlet myTabBarController *tabBarController;
	
	IBOutlet RootViewControllerLocalBrowser *rootViewControlleriPhone;
	IBOutlet DetailViewControllerIphone *detailViewControlleriPhone;
    IBOutlet RootViewControllerPlaylist *playlistVC;
	
	UIBackgroundTaskIdentifier bgTask;
}

//@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet ModizerWin *modizerWin;
@property (nonatomic, retain) IBOutlet RootViewControllerLocalBrowser *rootViewControlleriPhone;
@property (nonatomic, retain) IBOutlet RootViewControllerPlaylist *playlistVC;
@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewControlleriPhone;
@property (nonatomic, retain) IBOutlet myTabBarController *tabBarController;

-(int) isSlowDevice;
-(void) batteryChanged:(NSNotification*)notification;

@end

