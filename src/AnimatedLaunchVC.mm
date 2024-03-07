//
//  AnimatedLaunchVC.m
//  modizer
//
//  Created by Yohann Magnien David on 27/02/2024.
//

#import <Foundation/Foundation.h>
#import "AnimatedLaunchVC.h"

#define ANIMATION_GRID_CELL_SIZE 64
#define RADIAN(x) x*M_PI/180.0f


@interface AnimatedLaunchVC ()

@end

@implementation AnimatedLaunchVC

//@synthesize mpview;
@synthesize animation;
@synthesize localBrowserVC;

- (void)viewDidLoad {
    [super viewDidLoad];
    
    UIWindow *win=[UIApplication sharedApplication].keyWindow;
    
    // Do any additional setup after loading the view.
    int rows=round(win.bounds.size.height/ANIMATION_GRID_CELL_SIZE);
    int columns=round(win.bounds.size.width/ANIMATION_GRID_CELL_SIZE);
    //self.animation = [[SONFillAnimation alloc] initWithNumberOfRows:rows columns:columns animationType:SONFillAnimationTypeFoldIn];
    self.animation = [[SONFillAnimation alloc] initWithNumberOfRows:rows columns:columns animationType:SONFillAnimationTypeCustom];
}


- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
}

- (void)viewDidLayoutSubviews {
    
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
}

- (void)viewDidAppear:(BOOL)animated {
    animationDone=false;
    /*[UIView beginAnimations:NULL context:nil];
    [UIView setAnimationDuration:2.0];
    [self.view setAlpha:0];
    self.view.transform=CGAffineTransformMakeScale(2.0,2.0);
    //self.view.hidden=TRUE;
    [UIView commitAnimations];*/
    switch (arc4random()%5) {
        case 0:
            animation.direction = SONFillAnimationDirectionLeftToRightAndTopDown;
            break;
        case 1:
            animation.direction = SONFillAnimationDirectionLeftToRightAndBottomUp;
            break;
        case 2:
            animation.direction = SONFillAnimationDirectionRightToLeftAndTopDown;
            break;
        case 3:
            animation.direction = SONFillAnimationDirectionRightToLeftAndBottomUp;
            break;
        case 4:
            animation.direction = SONFillAnimationDirectionRandom;
            break;
    }
    
    
    animation.duration = 0.3f;
    animation.xDelay = 0.02f;
    animation.yDelay = 0.04f;
    animation.animateToHide=YES;
    animation.disableShadeAnimation = NO;

    
    CABasicAnimation *tAnimation1 = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
    tAnimation1.fromValue = [NSNumber numberWithFloat:1.0];
    tAnimation1.toValue = [NSNumber numberWithFloat:0.5];
    CABasicAnimation *tAnimation2 = [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
    tAnimation2.fromValue = [NSNumber numberWithFloat:RADIAN(0)];
    tAnimation2.toValue = [NSNumber numberWithFloat:RADIAN(45)];
    CABasicAnimation *tAnimation3 = [CABasicAnimation animationWithKeyPath:@"opacity"];
    tAnimation3.fromValue = [NSNumber numberWithFloat:1.0];
    tAnimation3.toValue = [NSNumber numberWithFloat:0.0];
    CAAnimationGroup *groupAnimation = [CAAnimationGroup animation];
    [groupAnimation setAnimations:[NSArray arrayWithObjects:tAnimation1, tAnimation2, tAnimation3, nil]];
    self.animation.transformAnimation = groupAnimation;
    
    //self.animation.anchorPoint = CGPointMake(0.5, 0.0);
   // [self.animation setRotationVector:1 :0 :0];
    [animation animateView:self.view completion:^{[self.localBrowserVC modizerIsLaunched];}];
}

-(void)viewDidDisappear:(BOOL)animated {
}

/*
 #pragma mark - Navigation
 
 // In a storyboard-based application, you will often want to do a little preparation before navigation
 - (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
 // Get the new view controller using [segue destinationViewController].
 // Pass the selected object to the new view controller.
 }
 */

@end
