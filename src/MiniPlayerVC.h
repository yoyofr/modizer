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
//    MiniPlayerView *mpview;
    
//    UIImage *coverImg;
//    UIImageView *coverView;
//    UIView *gestureAreaView;
    
//    UIView *songInfoView;
//    CBAutoScrollLabel *labelMain,*labelSub,*labelArtist;
//    UILabel *labelPrev,*labelNext;
//    UILabel *labelPrevEntry,*labelNextEntry;
//    UILabel *labelPlaylist;
//    UILabel *labelTime;
//    char labelTime_mode;
    
//    BButton *btnPlay,*btnPause;
    
//    bool darkMode;
    
//    NSTimer *repeatingTimer;
    
//    DetailViewControllerIphone *detailVC;
}

@property (nonatomic, strong) DetailViewControllerIphone *detailVC;
@property (nonatomic, strong) UIImage *coverImg;
@property (nonatomic, strong) MiniPlayerView *mpview;
@property (nonatomic, strong) CBAutoScrollLabel *labelMain,*labelSub,*labelArtist;
@property (nonatomic, strong) UILabel *labelPrev,*labelNext;
@property (nonatomic, strong) UILabel *labelPrevEntry,*labelNextEntry;
@property (nonatomic, strong) UILabel *labelPlaylist;
@property (nonatomic, strong) UILabel *labelTime;
@property (nonatomic, strong) UIImageView *coverView;
@property (nonatomic, strong) UIView *gestureAreaView;
@property (nonatomic, strong) UIView *songInfoView;
@property (nonatomic, strong) BButton *btnPlay,*btnPause;
@property (nonatomic, strong) NSTimer *repeatingTimer;
@property (nonatomic, assign) char labelTime_mode;
@property (nonatomic, assign) bool darkMode;
@property (nonatomic, assign) CGFloat previousLayoutWidth;

-(void) refreshCoverLabels;
-(void) refreshCoverView;
-(void) pushedPause;
-(void) pushedPlay;
-(void) pushedPlaylist;

@end

NS_ASSUME_NONNULL_END
