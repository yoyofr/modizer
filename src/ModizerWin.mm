//
//  ModizerApp.mm
//  modizer
//
//  Created by yoyofr on 15/08/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import "ModizerWin.h"

extern int shiftPressedL,shiftPressedR;
extern int move_cursorL,move_cursorR,keyDel;

@implementation ModizerWin
@synthesize detailViewControllerIphone;

- (NSArray *)keyCommands
{
    return @[ [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow  modifierFlags:0 action:@selector(leftPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow   modifierFlags:0 action:@selector(rightPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow   modifierFlags:UIKeyModifierAlternate action:@selector(leftAltPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow   modifierFlags:UIKeyModifierAlternate action:@selector(rightAltPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow   modifierFlags:UIKeyModifierCommand action:@selector(leftCmdPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow   modifierFlags:UIKeyModifierCommand action:@selector(rightCmdPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputUpArrow   modifierFlags:0 action:@selector(upPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputDownArrow   modifierFlags:0 action:@selector(downPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"1"   modifierFlags:0 action:@selector(key1Pressed)],
              [UIKeyCommand keyCommandWithInput:@"&"   modifierFlags:0 action:@selector(key1Pressed)],
              [UIKeyCommand keyCommandWithInput:@"1"   modifierFlags:UIKeyModifierAlternate action:@selector(key1AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"&"   modifierFlags:UIKeyModifierAlternate action:@selector(key1AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"2"   modifierFlags:0 action:@selector(key2Pressed)],
              [UIKeyCommand keyCommandWithInput:@"é"   modifierFlags:0 action:@selector(key2Pressed)],
              [UIKeyCommand keyCommandWithInput:@"2"   modifierFlags:UIKeyModifierAlternate action:@selector(key2AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"é"   modifierFlags:UIKeyModifierAlternate action:@selector(key2AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"3"   modifierFlags:0 action:@selector(key3Pressed)],
              [UIKeyCommand keyCommandWithInput:@"\""   modifierFlags:0 action:@selector(key3Pressed)],
              [UIKeyCommand keyCommandWithInput:@"3"   modifierFlags:UIKeyModifierAlternate action:@selector(key3AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"\""   modifierFlags:UIKeyModifierAlternate action:@selector(key3AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"4"   modifierFlags:0 action:@selector(key4Pressed)],
              [UIKeyCommand keyCommandWithInput:@"'"   modifierFlags:0 action:@selector(key4Pressed)],
              [UIKeyCommand keyCommandWithInput:@"4"   modifierFlags:UIKeyModifierAlternate action:@selector(key4AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"'"   modifierFlags:UIKeyModifierAlternate action:@selector(key4AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"5"   modifierFlags:0 action:@selector(key5Pressed)],
              [UIKeyCommand keyCommandWithInput:@"("   modifierFlags:0 action:@selector(key5Pressed)],
              [UIKeyCommand keyCommandWithInput:@"5"   modifierFlags:UIKeyModifierAlternate action:@selector(key5AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"("   modifierFlags:UIKeyModifierAlternate action:@selector(key5AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"6"   modifierFlags:0 action:@selector(key6Pressed)],
              [UIKeyCommand keyCommandWithInput:@"§"   modifierFlags:0 action:@selector(key6Pressed)],
              [UIKeyCommand keyCommandWithInput:@"6"   modifierFlags:UIKeyModifierAlternate action:@selector(key6AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"§"   modifierFlags:UIKeyModifierAlternate action:@selector(key6AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"7"   modifierFlags:0 action:@selector(key7Pressed)],
              [UIKeyCommand keyCommandWithInput:@"è"   modifierFlags:0 action:@selector(key7Pressed)],
              [UIKeyCommand keyCommandWithInput:@"7"   modifierFlags:UIKeyModifierAlternate action:@selector(key7AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"è"   modifierFlags:UIKeyModifierAlternate action:@selector(key7AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"8"   modifierFlags:0 action:@selector(key8Pressed)],
              [UIKeyCommand keyCommandWithInput:@"!"   modifierFlags:0 action:@selector(key8Pressed)],
              [UIKeyCommand keyCommandWithInput:@"8"   modifierFlags:UIKeyModifierAlternate action:@selector(key8AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"!"   modifierFlags:UIKeyModifierAlternate action:@selector(key8AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"9"   modifierFlags:0 action:@selector(key9Pressed)],
              [UIKeyCommand keyCommandWithInput:@"ç"   modifierFlags:0 action:@selector(key9Pressed)],
              [UIKeyCommand keyCommandWithInput:@"9"   modifierFlags:UIKeyModifierAlternate action:@selector(key9AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"ç"   modifierFlags:UIKeyModifierAlternate action:@selector(key9AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"0"   modifierFlags:0 action:@selector(key0Pressed)],
              [UIKeyCommand keyCommandWithInput:@"à"   modifierFlags:0 action:@selector(key0Pressed)],
              [UIKeyCommand keyCommandWithInput:@"0"   modifierFlags:UIKeyModifierAlternate action:@selector(key0AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"à"   modifierFlags:UIKeyModifierAlternate action:@selector(key0AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"i"   modifierFlags:0 action:@selector(keyIPressed)],
              [UIKeyCommand keyCommandWithInput:@"e"   modifierFlags:0 action:@selector(keyEPressed)],
              [UIKeyCommand keyCommandWithInput:@"f"   modifierFlags:0 action:@selector(keyFPressed)],
              [UIKeyCommand keyCommandWithInput:@"f"   modifierFlags:UIKeyModifierAlternate action:@selector(keyFAltPressed)],
              [UIKeyCommand keyCommandWithInput:@"s"   modifierFlags:0 action:@selector(keySPressed)],
              [UIKeyCommand keyCommandWithInput:@"s"   modifierFlags:UIKeyModifierAlternate action:@selector(keySAltPressed)],
              [UIKeyCommand keyCommandWithInput:@"h"   modifierFlags:0 action:@selector(keyHPressed)],
              [UIKeyCommand keyCommandWithInput:@"\r"   modifierFlags:0 action:@selector(enterPressed)],
              [UIKeyCommand keyCommandWithInput:@" "   modifierFlags:0 action:@selector(spacePressed)],
              
              [UIKeyCommand keyCommandWithInput:@"b"   modifierFlags:0 action:@selector(keyBPressed)],
              [UIKeyCommand keyCommandWithInput:@"m"   modifierFlags:0 action:@selector(keyMPressed)],
              [UIKeyCommand keyCommandWithInput:@"m"   modifierFlags:UIKeyModifierAlternate action:@selector(keyMAltPressed)],
              [UIKeyCommand keyCommandWithInput:@"o"   modifierFlags:0 action:@selector(keyOPressed)],
              [UIKeyCommand keyCommandWithInput:@"o"   modifierFlags:UIKeyModifierAlternate action:@selector(keyOAltPressed)],
              [UIKeyCommand keyCommandWithInput:@"v"   modifierFlags:0 action:@selector(keyVPressed)],
              [UIKeyCommand keyCommandWithInput:@"t"   modifierFlags:0 action:@selector(keyTPressed)],
              [UIKeyCommand keyCommandWithInput:@"t"   modifierFlags:UIKeyModifierAlternate action:@selector(keyTAltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"p"   modifierFlags:0 action:@selector(keyPPressed)],
              [UIKeyCommand keyCommandWithInput:@"n"   modifierFlags:0 action:@selector(keyNPressed)],
              [UIKeyCommand keyCommandWithInput:@"l"   modifierFlags:0 action:@selector(keyLPressed)],
              [UIKeyCommand keyCommandWithInput:@"a"   modifierFlags:0 action:@selector(keyAPressed)],
              [UIKeyCommand keyCommandWithInput:@"q"   modifierFlags:0 action:@selector(keyQPressed)],
              
              [UIKeyCommand keyCommandWithInput:UIKeyInputEscape   modifierFlags:0 action:@selector(keyESCPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputDelete   modifierFlags:0 action:@selector(keyDeletePressed)],
              [UIKeyCommand keyCommandWithInput:@"\t"  modifierFlags:0 action:@selector(keyTabPressed)],];
    
}
-(void)key1Pressed {
    [detailViewControllerIphone switchFX:1 change:1];
}
-(void)key1AltPressed {
    [detailViewControllerIphone switchFX:1 change:-1];
}
-(void)key2Pressed {
    [detailViewControllerIphone switchFX:2 change:1];
}
-(void)key2AltPressed {
    [detailViewControllerIphone switchFX:2 change:-1];
}
-(void)key3Pressed {
    [detailViewControllerIphone switchFX:3 change:1];
}
-(void)key3AltPressed {
    [detailViewControllerIphone switchFX:3 change:-1];
}
-(void)key4Pressed {
    [detailViewControllerIphone switchFX:4 change:1];
}
-(void)key4AltPressed {
    [detailViewControllerIphone switchFX:4 change:-1];
}
-(void)key5Pressed {
    [detailViewControllerIphone switchFX:5 change:1];
}
-(void)key5AltPressed {
    [detailViewControllerIphone switchFX:5 change:-1];
}
-(void)key6Pressed {
    [detailViewControllerIphone switchFX:6 change:1];
}
-(void)key6AltPressed {
    [detailViewControllerIphone switchFX:6 change:-1];
}
-(void)key7Pressed {
    [detailViewControllerIphone switchFX:7 change:1];
}
-(void)key7AltPressed {
    [detailViewControllerIphone switchFX:7 change:-1];
}
-(void)key8Pressed {
    [detailViewControllerIphone switchFX:8 change:1];
}
-(void)key8AltPressed {
    [detailViewControllerIphone switchFX:8 change:-1];
}
-(void)key9Pressed {
    [detailViewControllerIphone switchFX:9 change:1];
}
-(void)key9AltPressed {
    [detailViewControllerIphone switchFX:9 change:-1];
}
-(void)key0Pressed {
    [detailViewControllerIphone switchFX:0 change:1];
}
-(void)key0AltPressed {
    [detailViewControllerIphone switchFX:0 change:-1];
}
- (void)enterPressed{
    [detailViewControllerIphone oglViewSwitchFS];
}
- (void)keyVPressed{
    [detailViewControllerIphone mdSwitchVolBars];
}
- (void)keyBPressed{
    [detailViewControllerIphone mdSwitchFixedBar];
}
- (void)keyMPressed{
    [detailViewControllerIphone mdSwitchSpectrumBloom:1];
}
- (void)keyMAltPressed{
    [detailViewControllerIphone mdSwitchSpectrumBloom:-1];
}
- (void)keyOPressed{
    [detailViewControllerIphone mdSwitchLandscapeBloom:1];
}
- (void)keyOAltPressed{
    [detailViewControllerIphone mdSwitchLandscapeBloom:-1];
}
- (void)keyFPressed{
    [detailViewControllerIphone mdSwitchModPatternFont:1];
}
- (void)keyFAltPressed{
    [detailViewControllerIphone mdSwitchModPatternFont:-1];
}
- (void)keySPressed{
    [detailViewControllerIphone mdSwitchModPatternFontSize:1];
}
- (void)keySAltPressed{
    [detailViewControllerIphone mdSwitchModPatternFontSize:-1];
}
- (void)keyTPressed{
    [detailViewControllerIphone mdSwitchModPatternTheme:1];
}
- (void)keyTAltPressed{
    [detailViewControllerIphone mdSwitchModPatternTheme:-1];
}
- (void)keyIPressed{
    [detailViewControllerIphone mdInfoFX];
}
- (void)keyEPressed{
    [detailViewControllerIphone oglButtonPushed];
}
- (void)keyNPressed{
    [detailViewControllerIphone mdNextPreset];
}
- (void)keyPPressed{
    [detailViewControllerIphone mdPrevPreset];
}
- (void)keyHPressed{
    [detailViewControllerIphone mdSwitchFPSHud];
}
- (void)keyLPressed{
    [detailViewControllerIphone mdSwitchLockStatusPreset];
}
- (void)keyAPressed{
    [detailViewControllerIphone mdChangeFavoriteStatusPreset:0];
}
- (void)keyESCPressed{
    [detailViewControllerIphone mdOpenCloseMenu];
}
- (void)keyDeletePressed{
    [detailViewControllerIphone mdBackAction];
}
- (void)keyQPressed {
    [detailViewControllerIphone mdTestAsyncLoad];
}

- (UIViewController *) getVisibleViewControllerFrom:(UIViewController *) vc {
    if ([vc isKindOfClass:[UINavigationController class]]) {
        return [self getVisibleViewControllerFrom:[((UINavigationController *) vc) visibleViewController]];
    } else if ([vc isKindOfClass:[UITabBarController class]]) {
        return [self getVisibleViewControllerFrom:[((UITabBarController *) vc) selectedViewController]];
    } else {
        if (vc.presentedViewController) {
            return [self getVisibleViewControllerFrom:vc.presentedViewController];
        } else {
            return vc;
        }
    }
}

- (UIViewController *)visibleViewController {
    UIViewController *rootViewController = self.rootViewController;
    return [self getVisibleViewControllerFrom:rootViewController];
}

- (void)keyTabPressed{
    UIViewController *currentVC=[self visibleViewController];
    if (currentVC) {
        if ([currentVC respondsToSelector:@selector(goPlayer)]) [currentVC performSelector:@selector(goPlayer)];
    }
}

-(void)leftPressed {
    [detailViewControllerIphone jumpSeekBwd];
}
-(void)rightPressed {
    [detailViewControllerIphone jumpSeekFwd];
}
-(void)leftCmdPressed {
    [detailViewControllerIphone playPrev];
}
-(void)rightCmdPressed {
    [detailViewControllerIphone playNext];
}
-(void)leftAltPressed {
    if ((detailViewControllerIphone.mplayer.mod_subsongs>1)&&
        (detailViewControllerIphone.mplayer.mod_currentsub>detailViewControllerIphone.mplayer.mod_minsub)&&(detailViewControllerIphone.mOnlyCurrentSubEntry==0))
        [detailViewControllerIphone playPrevSub]; //should handle sub ?
    else {//no more subsongs, check if within an archive to play prev entry
        if ([detailViewControllerIphone.mplayer isArchive]&&([detailViewControllerIphone.mplayer getArcEntriesCnt]>1)&&([detailViewControllerIphone.mplayer getArcIndex]>0)&&(detailViewControllerIphone.mOnlyCurrentEntry==0)) {
            [detailViewControllerIphone.mplayer selectPrevArcEntry];
            [detailViewControllerIphone play_loadArchiveModule];
        } else [detailViewControllerIphone play_prevEntry];
    }
}
-(void)rightAltPressed {
    //1st check if there are more subsongs
    if ((detailViewControllerIphone.mplayer.mod_subsongs>1)&&
        (detailViewControllerIphone.mplayer.mod_currentsub<detailViewControllerIphone.mplayer.mod_maxsub)&&(detailViewControllerIphone.mOnlyCurrentSubEntry==0))
        [detailViewControllerIphone playNextSub];
    else {
        //no more subsongs, check if within an archive to play next entry
        
        if ([detailViewControllerIphone.mplayer isArchive]&&([detailViewControllerIphone.mplayer getArcEntriesCnt]>1)&&(detailViewControllerIphone.mOnlyCurrentEntry==0)) {
            if ([detailViewControllerIphone.mplayer selectNextArcEntry]<0) [detailViewControllerIphone play_nextEntry];
            else [detailViewControllerIphone play_loadArchiveModule];
        } else [detailViewControllerIphone play_nextEntry];
    }
}

-(void)upPressed {
    [detailViewControllerIphone restartCurrent];
}
-(void)downPressed {
    [self rightAltPressed];
}
-(void)spacePressed {
    if (detailViewControllerIphone.mPaused) [detailViewControllerIphone playPushed:nil];
    else [detailViewControllerIphone pausePushed:nil];
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    bool _dontForwardEvent=false;
    if (@available(iOS 13.4, *)) {
        for (UIPress *press in presses) {
            UIKey *key=press.key;
            if (key.keyCode==UIKeyboardHIDUsageKeyboardRightShift) {
                [detailViewControllerIphone mdShiftMode:0];
                shiftPressedR=1;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardLeftShift) {
                shiftPressedL=1;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardRightArrow) {
                move_cursorR=1;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardLeftArrow) {
                move_cursorL=1;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardDeleteForward) {
                keyDel=1;
            }
        }
    }
    if (!_dontForwardEvent) [super pressesEnded:presses withEvent:event];
}
- (void)pressesBegan:(NSSet<UIPress *> *)presses
           withEvent:(UIPressesEvent *)event {
    bool _dontForwardEvent=false;
    if (@available(iOS 13.4, *)) {
        for (UIPress *press in presses) {
            UIKey *key=press.key;
            if (key.keyCode==UIKeyboardHIDUsageKeyboardRightShift) {
                [detailViewControllerIphone mdShiftMode:1];
                shiftPressedR=2;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardLeftShift) {
                shiftPressedL=2;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardRightArrow) {
                move_cursorR=2;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardLeftArrow) {
                move_cursorL=2;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardDeleteForward) {
                keyDel=2;
            }
        }
    }
    if (!_dontForwardEvent) [super pressesBegan:presses withEvent:event];
}


- (void)remoteControlReceivedWithEvent:(UIEvent *)event {
	if (event.subtype == UIEventSubtypeRemoteControlTogglePlayPause) {
		if (detailViewControllerIphone.mPaused) {
			detailViewControllerIphone.mPaused=0;
			[detailViewControllerIphone.mplayer Pause:NO];
		} else {
			detailViewControllerIphone.mPaused=1;
			[detailViewControllerIphone.mplayer Pause:YES];
		}
	}
	if (event.subtype == UIEventSubtypeRemoteControlPlay) {
		if (detailViewControllerIphone.mPaused) {
			detailViewControllerIphone.mPaused=0;
			[detailViewControllerIphone.mplayer Pause:NO];
		}
	}
	if (event.subtype == UIEventSubtypeRemoteControlPause) {
		if (detailViewControllerIphone.mPaused==0) {
			detailViewControllerIphone.mPaused=1;
			[detailViewControllerIphone.mplayer Pause:YES];
		}
	}
	if (event.subtype == UIEventSubtypeRemoteControlStop) {
		if (detailViewControllerIphone.mPaused==0) {
			detailViewControllerIphone.mPaused=1;
			[detailViewControllerIphone.mplayer Pause:YES];
		}
	}
	if (event.subtype == UIEventSubtypeRemoteControlNextTrack) {
        //1st check if there are more subsongs
        if ((detailViewControllerIphone.mplayer.mod_subsongs>1)&&
			(detailViewControllerIphone.mplayer.mod_currentsub<detailViewControllerIphone.mplayer.mod_maxsub)&&(detailViewControllerIphone.mOnlyCurrentSubEntry==0))
            [detailViewControllerIphone playNextSub];
        else {
            //no more subsongs, check if within an archive to play next entry
            
            if ([detailViewControllerIphone.mplayer isArchive]&&([detailViewControllerIphone.mplayer getArcEntriesCnt]>1)&&(detailViewControllerIphone.mOnlyCurrentEntry==0)) {
                if ([detailViewControllerIphone.mplayer selectNextArcEntry]<0) [detailViewControllerIphone play_nextEntry];
                else [detailViewControllerIphone play_loadArchiveModule];
            } else [detailViewControllerIphone play_nextEntry];
        }
	}
	if (event.subtype == UIEventSubtypeRemoteControlPreviousTrack) {
        //1st check if there are more subsongs
		if ((detailViewControllerIphone.mplayer.mod_subsongs>1)&&
			(detailViewControllerIphone.mplayer.mod_currentsub>detailViewControllerIphone.mplayer.mod_minsub)&&(detailViewControllerIphone.mOnlyCurrentSubEntry==0))
			[detailViewControllerIphone playPrevSub]; //should handle sub ?
        else {//no more subsongs, check if within an archive to play prev entry
            if ([detailViewControllerIphone.mplayer isArchive]&&([detailViewControllerIphone.mplayer getArcEntriesCnt]>1)&&([detailViewControllerIphone.mplayer getArcIndex]>0)&&(detailViewControllerIphone.mOnlyCurrentEntry==0)) {
                [detailViewControllerIphone.mplayer selectPrevArcEntry];
                [detailViewControllerIphone play_loadArchiveModule];
            } else [detailViewControllerIphone play_prevEntry];
        }
	}
    
    /*UIEventSubtypeRemoteControlBeginSeekingBackward = 106,
    UIEventSubtypeRemoteControlEndSeekingBackward   = 107,
    UIEventSubtypeRemoteControlBeginSeekingForward  = 108,
    UIEventSubtypeRemoteControlEndSeekingForward    = 109,*/
}


- (BOOL)canBecomeFirstResponder {
	return YES;
}

@end
