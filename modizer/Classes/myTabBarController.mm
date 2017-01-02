//
//  myTabBarController.m
//  modizer4
//
//  Created by Yohann Magnien on 14/06/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import "myTabBarController.h"


@implementation myTabBarController
@synthesize detailViewControlleriPhone;
//@synthesize aboutViewController;
@synthesize webBrowser;
@synthesize rootViewControllerIphone;

- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
    //    return UIInterfaceOrientationMaskAllButUpsideDown;
}



- (BOOL)shouldAutorotate {
    [self shouldAutorotateToInterfaceOrientation:self.interfaceOrientation];
    return YES;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	
	[detailViewControlleriPhone shouldAutorotateToInterfaceOrientation:interfaceOrientation];
//	[aboutViewController shouldAutorotateToInterfaceOrientation:interfaceOrientation];
	[webBrowser shouldAutorotateToInterfaceOrientation:interfaceOrientation];
	
	//[rootViewControllerIphone shouldAutorotateToInterfaceOrientation:interfaceOrientation];
	
	return YES;
}

/*- (void)viewDidLoad {
	[super viewDidLoad];
	
//    UINavigationController *moreController = self.moreNavigationController;
//    moreController.navigationBar.barStyle = UIBarStyleBlackOpaque;
//	moreController.navigationBar.hidden=TRUE;
}
*/
@end
