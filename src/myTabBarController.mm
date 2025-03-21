//
//  myTabBarController.m
//  modizer4
//
//  Created by Yohann Magnien on 14/06/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import "myTabBarController.h"
#import "TTFadeAnimator.h"


@implementation myTabBarController
@synthesize detailViewControlleriPhone;
//@synthesize aboutViewController;
@synthesize webBrowser;
@synthesize rootViewControllerIphone;

- (UIViewController *)visibleViewController:(UIViewController *)rootViewController
{
    if ([rootViewController isKindOfClass:[UITabBarController class]])
    {
        UIViewController *selectedViewController = ((UITabBarController *)rootViewController).selectedViewController;

        return [self visibleViewController:selectedViewController];
    }
    if ([rootViewController isKindOfClass:[UINavigationController class]])
    {
        UIViewController *lastViewController = [[((UINavigationController *)rootViewController) viewControllers] lastObject];

        return [self visibleViewController:lastViewController];
    }
    
    if (rootViewController.presentedViewController == nil)
    {
        return rootViewController;
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UINavigationController class]])
    {
        UINavigationController *navigationController = (UINavigationController *)rootViewController.presentedViewController;
        UIViewController *lastViewController = [[navigationController viewControllers] lastObject];

        return [self visibleViewController:lastViewController];
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UITabBarController class]])
    {
        UITabBarController *tabBarController = (UITabBarController *)rootViewController.presentedViewController;
        UIViewController *selectedViewController = tabBarController.selectedViewController;

        return [self visibleViewController:selectedViewController];
    }

    UIViewController *presentedViewController = (UIViewController *)rootViewController.presentedViewController;

    return [self visibleViewController:presentedViewController];
}


- (UIStatusBarStyle)preferredStatusBarStyle {    
    return UIStatusBarStyleDefault;
}

- (UIViewController *)childViewControllerForStatusBarStyle {
    UIViewController *vc=[self visibleViewController:self];
    return vc;
}

- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
    //    return UIInterfaceOrientationMaskAllButUpsideDown;
}

- (BOOL)shouldAutorotate {
    [self shouldAutorotateToInterfaceOrientation:self.interfaceOrientation];
    return YES;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	
	//[detailViewControlleriPhone shouldAutorotateToInterfaceOrientation:interfaceOrientation];
//	[aboutViewController shouldAutorotateToInterfaceOrientation:interfaceOrientation];
	//[webBrowser shouldAutorotateToInterfaceOrientation:interfaceOrientation];
	
	//[rootViewControllerIphone shouldAutorotateToInterfaceOrientation:interfaceOrientation];
	
	return YES;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    self.navigationController.delegate = self;
    
    [self setNeedsStatusBarAppearanceUpdate];
}

- (void)viewDidLoad {
	[super viewDidLoad];    
    self.navigationController.delegate = self;
    
    self.view.backgroundColor = [UIColor clearColor];
    
//    UINavigationController *moreController = self.moreNavigationController;
//    moreController.navigationBar.barStyle = UIBarStyleBlackOpaque;
//	moreController.navigationBar.hidden=TRUE;
    
    
}

@end
