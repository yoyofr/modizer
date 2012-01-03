//
//  myNavViewController.h
//  modizer4
//
//  Created by yoyofr on 7/14/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"
@class DetailViewControllerIphone;


@interface myNavViewController : UIViewController {
	IBOutlet DetailViewControllerIphone *detailView;
	
	IBOutlet UIBarButtonItem *playerButton;
}

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailView;
@property (nonatomic, retain) IBOutlet UIBarButtonItem *playerButton;

-(IBAction) goPlayer;

@end
