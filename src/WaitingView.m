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
        indView.translatesAutoresizingMaskIntoConstraints=false;
        [self addSubview:indView];
        [indView startAnimating];
        
        // center align
        [self addConstraint:[NSLayoutConstraint constraintWithItem:indView attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
        [self addConstraint:[NSLayoutConstraint constraintWithItem:indView attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
        
        lblDetail=[[UILabel alloc] initWithFrame:CGRectMake(10,90,80+20,30)];
        lblDetail.text=@"";
        lblDetail.backgroundColor=[UIColor blackColor];
        lblDetail.numberOfLines=0;
        lblDetail.opaque=YES;
        lblDetail.textColor=[UIColor whiteColor];
        lblDetail.textAlignment=NSTextAlignmentCenter;
        lblDetail.font=[UIFont italicSystemFontOfSize:16];
        lblDetail.translatesAutoresizingMaskIntoConstraints=false;
        [self addSubview:lblDetail];
        
        // center align
        [self addConstraint:[NSLayoutConstraint constraintWithItem:lblDetail attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
        [self addConstraint:[NSLayoutConstraint constraintWithItem:lblDetail attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:indView attribute:NSLayoutAttributeBottom multiplier:1.0 constant:4]];
        
        lblTitle=[[UILabel alloc] initWithFrame:CGRectMake(10,10,80+20,20)];
        lblTitle.text=@"";
        lblTitle.backgroundColor=[UIColor blackColor];
        lblTitle.numberOfLines=0;
        lblTitle.opaque=YES;
        lblTitle.textColor=[UIColor whiteColor];
        lblTitle.textAlignment=NSTextAlignmentCenter;
        lblTitle.translatesAutoresizingMaskIntoConstraints=false;
        [self addSubview:lblTitle];
        
        // center align
        [self addConstraint:[NSLayoutConstraint constraintWithItem:lblTitle attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
        [self addConstraint:[NSLayoutConstraint constraintWithItem:lblTitle attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeTop multiplier:1.0 constant:8]];
        
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
        [self addConstraint:[NSLayoutConstraint constraintWithItem:btnStopCurrentAction attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
        [self addConstraint:[NSLayoutConstraint constraintWithItem:btnStopCurrentAction attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeBottom multiplier:1.0 constant:-4]];
        
        
        progressView = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
        progressView.progressTintColor = [UIColor colorWithRed:200.0/255 green:200.0/255 blue:255.0/255 alpha:1.0];
        [[progressView layer]setFrame:CGRectMake(0, 0,128, 10)];
        //[[progressView layer]setBorderColor:[UIColor whiteColor].CGColor];
        progressView.trackTintColor = [UIColor colorWithRed:40.0/255 green:40.0/255 blue:80.0/255 alpha:1.0];
        [progressView setProgress:(float)(0) animated:NO];  ///15

        //[[progressView layer]setCornerRadius:progressView.frame.size.width / 2];
        //[[progressView layer]setBorderWidth:1];
        //[[progressView layer]setCornerRadius:10];
        //[[progressView layer]setMasksToBounds:TRUE];
        //progressView.clipsToBounds = YES;
        
        [self addSubview:progressView];
        progressView.translatesAutoresizingMaskIntoConstraints = false;
        progressView.hidden=true;
        
        views = NSDictionaryOfVariableBindings(progressView);
        // width constraint
        [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[progressView(128)]" options:0 metrics:nil views:views]];
        // height constraint
        [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[progressView(10)]" options:0 metrics:nil views:views]];
        // center align
        [self addConstraint:[NSLayoutConstraint constraintWithItem:progressView attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
        [self addConstraint:[NSLayoutConstraint constraintWithItem:progressView attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:self attribute:NSLayoutAttributeBottom multiplier:1.0 constant:-44]];
        
    }
    return self;
}

-(void)setProgress:(double)prg {
    [progressView setProgress:prg animated:YES];
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

-(void)hideProgress {
    progressView.hidden=true;
}

-(void)showProgress {
    [progressView setProgress:0 animated:NO];
    progressView.hidden=false;
}

- (UIViewController *) firstAvailableUIViewController {
    UIResponder *responder = [self nextResponder];
    while (responder != nil) {
        if ([responder isKindOfClass:[UIViewController class]]) {
            return (UIViewController *)responder;
        }
        responder = [responder nextResponder];
    }
    return nil;
}

-(void) pushedCancel {
    //    NSLog(@"cancel");
    btnStopCurrentActionPending=true;
    UIViewController *vc=[self firstAvailableUIViewController];
    if ([vc respondsToSelector:@selector(cancelPushed)]) {
        [vc performSelector:@selector(cancelPushed)];
    }
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
