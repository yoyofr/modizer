//
//  AnimatedLaunchVC.m
//  modizer
//
//  Created by Yohann Magnien David on 27/02/2024.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"
#import "SONFillAnimation.h"
#import "RootViewControllerLocalBrowser.h"

@class DetailViewControllerIphone;
@class RootViewControllerLocalBrowser;
@class myTabBarController;

@interface AnimatedLaunchVC : UIViewController {
    bool darkMode;
    bool animationDone;
    SONFillAnimation *animation;
    RootViewControllerLocalBrowser *localBrowserVC;
    myTabBarController *tabVC;
}

@property (nonatomic, retain) RootViewControllerLocalBrowser *localBrowserVC;
@property (nonatomic, retain) myTabBarController *tabVC;
@property (nonatomic, strong) SONFillAnimation *animation;
@property bool animationDone;

@end

