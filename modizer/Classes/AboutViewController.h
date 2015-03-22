//
//  AboutViewController.h
//  modizer
//
//  Created by Yohann Magnien on 16/09/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"


@class DetailViewControllerIphone;

@interface AboutViewController : UIViewController {
	IBOutlet DetailViewControllerIphone *detailViewControllerIphone;
	IBOutlet UITextView *textView;
}
@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewControllerIphone;
@property (nonatomic, retain) IBOutlet UITextView *textView;

-(IBAction) goPlayer;

@end
