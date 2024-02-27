//
//  myTabBarController.h
//  modizer4
//
//  Created by Yohann Magnien on 14/06/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"
//#import "AboutViewController.h"
#import "WebBrowser.h"
#import "RootViewControllerLocalBrowser.h"


@class DetailViewControllerIphone;
@class RootViewControllerLocalBrowser;
//@class AboutViewController;
@class AnimatedLaunchVC;

@interface myTabBarController : UITabBarController <UINavigationControllerDelegate,UITabBarDelegate> {
	IBOutlet DetailViewControllerIphone *detailViewControlleriPhone;
	//IBOutlet AboutViewController *aboutViewController;
	IBOutlet WebBrowser *webBrowser;
	IBOutlet RootViewControllerLocalBrowser *rootViewControllerIphone;
    
    AnimatedLaunchVC *animatedLaunchVC;
}

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewControlleriPhone;
//@property (nonatomic, retain) IBOutlet AboutViewController *aboutViewController;
@property (nonatomic, retain) IBOutlet WebBrowser *webBrowser;
@property (nonatomic, retain) IBOutlet RootViewControllerLocalBrowser *rootViewControllerIphone;

@end
