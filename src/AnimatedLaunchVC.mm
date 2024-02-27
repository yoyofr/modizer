//
//  AnimatedLaunchVC.m
//  modizer
//
//  Created by Yohann Magnien David on 27/02/2024.
//

#import <Foundation/Foundation.h>
#import "AnimatedLaunchVC.h"

@interface AnimatedLaunchVC ()

@end

@implementation AnimatedLaunchVC

//@synthesize mpview;

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
}

- (void)viewDidLayoutSubviews {
    
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
}

- (void)viewDidAppear:(BOOL)animated {
    
    [UIView beginAnimations:NULL context:nil];
    [UIView setAnimationDuration:2.0];
    [self.view setAlpha:0];
    self.view.transform=CGAffineTransformMakeScale(2.0,2.0);
    self.view.hidden=TRUE;
    [UIView commitAnimations];    
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
