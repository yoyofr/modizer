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
-(void) showWaiting{
    waitingView.hidden=FALSE;
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
