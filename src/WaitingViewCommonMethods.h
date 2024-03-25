//
//  WaitingViewCommonMethods.h
//  modizer
//
//  Created by Yohann Magnien on 15/04/2021.
//

#ifndef WaitingViewCommonMethods_h
#define WaitingViewCommonMethods_h

-(void) flushMainLoop {
    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate date]];
//    NSDate* futureDate = [NSDate dateWithTimeInterval:0.001f sinceDate:[NSDate date]];
//    [[NSRunLoop currentRunLoop] runUntilDate:futureDate];
}

-(void) showWaitingLoading{
    [waitingView hideCancel];
    [waitingView setDetail:@""];
    [waitingView setTitle:NSLocalizedString(@"Loading","")];
    waitingView.hidden=FALSE;
    [self flushMainLoop];
}

-(void) showWaitingBlank{
    [waitingView hideCancel];
    [waitingView setDetail:@""];
    [waitingView setTitle:@""];
    waitingView.hidden=FALSE;
    [self flushMainLoop];
}

-(void) showWaitingWithCancel {
    [waitingView showCancel];
    [waitingView setDetail:@""];
    [waitingView setTitle:@""];
    waitingView.hidden=FALSE;
    [waitingView resetCancelStatus];
    [self flushMainLoop];
}

-(void) hideWaitingCancel {
    [waitingView hideCancel];
}
-(void) showWaitingCancel {
    [waitingView showCancel];
}

-(void) setProgressWaiting:(NSNumber*)progress {
    [waitingView setProgress:[progress doubleValue]];
}

-(void) hideWaitingProgress {
    [waitingView hideProgress];
}
-(void) showWaitingProgress {
    [waitingView showProgress];
}


-(void) showWaiting{
    waitingView.hidden=FALSE;
    waitingView.layer.zPosition=MAXFLOAT;
    [waitingView setNeedsLayout];
    [waitingView layoutIfNeeded];
    [self.view bringSubviewToFront:waitingView];
    /*[self.view setNeedsLayout];
    [self.view layoutIfNeeded];*/
    
    /*[NSThread sleepForTimeInterval:0.1f];
    [self flushMainLoop];
    [NSThread sleepForTimeInterval:0.1f];
    [self flushMainLoop];*/
}
-(void) hideWaiting{
    waitingView.hidden=TRUE;    
}
-(bool) isCancelPending {
    return [waitingView isCancelPending];
}
-(void) resetCancelStatus {
    [waitingView resetCancelStatus];
}
-(void) updateWaitingDetail:(NSString *)text {
    [waitingView setDetail:text];
}
-(void) updateWaitingTitle:(NSString *)text {
    [waitingView setTitle:text];
}


#endif /* WaitingViewCommonMethods_h */
