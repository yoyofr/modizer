//
//  WaitingView.h
//  modizer
//
//  Created by Yohann Magnien on 13/04/2021.
//

#import <UIKit/UIKit.h>
#import "BButton.h"

NS_ASSUME_NONNULL_BEGIN

@interface WaitingView : UIView {
    UILabel *lblDetail,*lblTitle;
    UIProgressView *progressView;
    bool btnStopCurrentActionPending;
    BButton *btnStopCurrentAction;
}
@property (nonatomic,retain) UILabel *lblDetail,*lblTitle;
@property (nonatomic,retain) UIProgressView *progressView;
@property (nonatomic,retain) BButton *btnStopCurrentAction;
@property bool btnStopCurrentActionPending;

-(void)setDetail:(NSString*)text;

-(void)setTitle:(NSString*)text;

-(void)setProgress:(double)prg;

-(void)hideCancel;

-(void)showCancel;

-(void) pushedCancel;

-(bool) isCancelPending;

-(void) resetCancelStatus;

-(void)hideProgress;

-(void)showProgress;


@end

NS_ASSUME_NONNULL_END
