//
//  AboutViewController.h
//  modizer
//
//  Created by Yohann Magnien on 16/09/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"

@class DetailViewControllerIphone;

@interface AboutViewController : UIViewController {
	IBOutlet DetailViewControllerIphone *detailViewController;
	IBOutlet UITextView *textView;
    
    bool darkMode;
    bool forceReloadCells;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    WaitingView *waitingView;
}
@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UITextView *textView;

-(IBAction) goPlayer;

-(void) updateMiniPlayer;

@end
