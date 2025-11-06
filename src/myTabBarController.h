//
//  myTabBarController.h
//  modizer4
//
//  Created by Yohann Magnien on 14/06/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
//#import "AboutViewController.h"
#import "WebBrowser.h"
#import "OnlineViewController.h"
#import "RootViewControllerLocalBrowser.h"


@class DetailViewControllerIphone;
@class RootViewControllerLocalBrowser;
@class RootViewControllerPlaylist;
@class OnlineViewController;
@class SearchViewController;
@class MoreViewController;
@class AboutViewController;

@class AnimatedLaunchVC;

@interface myTabBarController : UITabBarController <UINavigationControllerDelegate,UITabBarDelegate> {
    DetailViewControllerIphone *detailViewControllerIphone;
    RootViewControllerPlaylist *playlistVC;
    RootViewControllerLocalBrowser *rootViewControllerIphone;
    OnlineViewController *onlineVC;
    SearchViewController *searchVC;
    MoreViewController *moreVC;
	//IBOutlet AboutViewController *aboutViewController;
    WebBrowser *webBrowser;
    DownloadViewController *downloadVC;
    AboutViewController *aboutVC;
    
    AnimatedLaunchVC *animatedLaunchVC;
    
    CarPlayAndRemoteManagement *cpMngt;
}

@property (nonatomic, retain) DetailViewControllerIphone *detailViewControllerIphone;
@property (nonatomic, retain) RootViewControllerLocalBrowser *rootViewControllerIphone;
@property (nonatomic, retain) RootViewControllerPlaylist *playlistVC;
@property (nonatomic, retain) OnlineViewController *onlineVC;
@property (nonatomic, retain) SearchViewController *searchVC;
@property (nonatomic, retain) MoreViewController *moreVC;
@property (nonatomic, retain) AboutViewController *aboutVC;

@property (nonatomic, retain) WebBrowser *webBrowser;
@property (nonatomic, retain) DownloadViewController *downloadVC;

@property (nonatomic, retain) AnimatedLaunchVC *animatedLaunchVC;
@property (nonatomic, retain) CarPlayAndRemoteManagement *cpMngt;


@end
