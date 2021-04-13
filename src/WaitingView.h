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
    bool btnStopCurrentActionPending;
    BButton *btnStopCurrentAction;
}

-(void)setDetail:(NSString*)text;

-(void)setTitle:(NSString*)text;

-(void)hideCancel;

-(void)showCancel;

-(void) pushedCancel;

-(bool) isCancelPending;

-(void) resetCancelStatus;


@end

NS_ASSUME_NONNULL_END
