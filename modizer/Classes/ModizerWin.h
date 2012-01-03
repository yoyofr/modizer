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
	IBOutlet DetailViewControllerIphone *detailViewControllerIphone;
}
@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewControllerIphone;

@end
