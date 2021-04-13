//
//  MiniPlayerVC.h
//  modizer
//
//  Created by Yohann Magnien on 12/04/2021.
//

#import <UIKit/UIKit.h>
#import "MiniPlayerView.h"
#import "BButton.h"
#import "CBAutoScrollLabel.h"

NS_ASSUME_NONNULL_BEGIN

@class DetailViewControllerIphone;

@interface MiniPlayerVC : UIViewController {
    MiniPlayerView *mpview;
    UIImageView *coverView;
    UIImage *coverImg;
    UIView *gestureAreaView;
    
    CBAutoScrollLabel *labelMain,*labelSub;
    UILabel *labelPlaylist;
    UILabel *labelTime;
    char labelTime_mode;
    
    BButton *btnPlay,*btnPause;
    
    bool darkMode;
    
    NSTimer *repeatingTimer;
    
    DetailViewControllerIphone *detailVC;
}

@property (nonatomic, retain) DetailViewControllerIphone *detailVC;
@property (nonatomic, retain) UIImage *coverImg;
@property (nonatomic, retain) MiniPlayerView *mpview;

-(void) refreshCoverLabels;
-(void) pushedPause;
-(void) pushedPlay;

@end

NS_ASSUME_NONNULL_END
