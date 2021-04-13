//
//  WaitingView.m
//  modizer
//
//  Created by Yohann Magnien on 13/04/2021.
//

#import "WaitingView.h"

@implementation WaitingView

- (instancetype)init
{
    self = [super init];
    if (self) {        
        self.backgroundColor=[UIColor blackColor];//[UIColor colorWithRed:0 green:0 blue:0 alpha:0.8f];
        self.opaque=YES;
        self.hidden=FALSE;
        self.layer.cornerRadius=20;
        
        UIActivityIndicatorView *indView=[[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(50-20+10,50-30+20+3,40,40)];
        indView.activityIndicatorViewStyle=UIActivityIndicatorViewStyleWhiteLarge;
        [self addSubview:indView];
        lblDetail=[[UILabel alloc] initWithFrame:CGRectMake(10,90,80+20,20)];
        lblDetail.text=@"";
        lblDetail.backgroundColor=[UIColor blackColor];
        lblDetail.opaque=YES;
        lblDetail.textColor=[UIColor whiteColor];
        lblDetail.textAlignment=NSTextAlignmentCenter;
        lblDetail.font=[UIFont italicSystemFontOfSize:16];
        [self addSubview:lblDetail];
        
        lblTitle=[[UILabel alloc] initWithFrame:CGRectMake(10,10,80+20,20)];
        lblTitle.text=@"";
        lblTitle.backgroundColor=[UIColor blackColor];
        lblTitle.opaque=YES;
        lblTitle.textColor=[UIColor whiteColor];
        lblTitle.textAlignment=NSTextAlignmentCenter;
        [self addSubview:lblTitle];
        
        [indView startAnimating];
        
        self.translatesAutoresizingMaskIntoConstraints = NO;
        
        
        btnStopCurrentActionPending=false;
        btnStopCurrentAction=[[BButton alloc] initWithFrame:CGRectMake(0,0,64,32) type:BButtonTypeDanger style:BButtonStyleBootstrapV2 icon:FAIconVolumeDown fontSize:12];
        [btnStopCurrentAction setTitle:NSLocalizedString(@"Cancel",@"") forState:UIControlStateNormal];
        [btnStopCurrentAction addTarget:self action:@selector(pushedCancel) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:btnStopCurrentAction];
        //[btnStopCurrentAction addAwesomeIcon:FAIconVolumeUp beforeTitle:true];
        btnStopCurrentAction.translatesAutoresizingMaskIntoConstraints = false;
        btnStopCurrentAction.hidden=true;
        
        NSDictionary *views = NSDictionaryOfVariableBindings(btnStopCurrentAction);
        // width constraint
        [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[btnStopCurrentAction(64)]" options:0 metrics:nil views:views]];
        // height constraint
        [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[btnStopCurrentAction(32)]" options:0 metrics:nil views:views]];
        // center align
        [self addConstraint:[NSLayoutConstraint constraintWithItem:self attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:btnStopCurrentAction attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
        [self addConstraint:[NSLayoutConstraint constraintWithItem:lblDetail attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:btnStopCurrentAction attribute:NSLayoutAttributeTop multiplier:1.0 constant:0]];
    }
    return self;
}

-(void)setDetail:(NSString*)text {
    lblDetail.text=text;
    [self setNeedsLayout];
}

-(void)setTitle:(NSString*)text {
    lblTitle.text=text;
    [self setNeedsLayout];
}

-(void)hideCancel {
    btnStopCurrentAction.hidden=true;
}

-(void)showCancel {
    btnStopCurrentAction.hidden=false;
}

-(void) pushedCancel {
    btnStopCurrentActionPending=true;
}

-(bool) isCancelPending {
    return btnStopCurrentActionPending;
}

-(void) resetCancelStatus {
    btnStopCurrentActionPending=false;
}

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code
}
*/

@end
