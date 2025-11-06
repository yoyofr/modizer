//
//  ModizerApp.h
//  modizer
//
//  Created by yoyofr on 15/08/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"

@class DetailViewControllerIphone;

@interface ModizerWin : UIWindow  {
    DetailViewControllerIphone *detailViewControllerIphone;
}
@property (nonatomic, retain) DetailViewControllerIphone *detailViewControllerIphone;

// Scene-based initializer for iOS 13+
- (instancetype)initWithWindowScene:(UIWindowScene *)windowScene NS_AVAILABLE_IOS(13.0);

@end
