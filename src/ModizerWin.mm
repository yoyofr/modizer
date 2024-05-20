//
//  ModizerApp.mm
//  modizer
//
//  Created by yoyofr on 15/08/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import "ModizerWin.h"


@implementation ModizerWin
@synthesize detailViewControllerIphone;

- (NSArray *)keyCommands
{
    return @[ [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow  modifierFlags:0 action:@selector(leftPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow   modifierFlags:0 action:@selector(rightPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow   modifierFlags:UIKeyModifierShift action:@selector(leftShiftPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow   modifierFlags:UIKeyModifierShift action:@selector(rightShiftPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow   modifierFlags:UIKeyModifierCommand action:@selector(leftCmdPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow   modifierFlags:UIKeyModifierCommand action:@selector(rightCmdPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputUpArrow   modifierFlags:0 action:@selector(upPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputDownArrow   modifierFlags:0 action:@selector(downPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"1"   modifierFlags:0 action:@selector(key1Pressed)],
              [UIKeyCommand keyCommandWithInput:@"&"   modifierFlags:0 action:@selector(key1Pressed)],
              
              [UIKeyCommand keyCommandWithInput:@"2"   modifierFlags:0 action:@selector(key2Pressed)],
              [UIKeyCommand keyCommandWithInput:@"é"   modifierFlags:0 action:@selector(key2Pressed)],
              
              [UIKeyCommand keyCommandWithInput:@"3"   modifierFlags:0 action:@selector(key3Pressed)],
              [UIKeyCommand keyCommandWithInput:@"\""   modifierFlags:0 action:@selector(key3Pressed)],
              
              [UIKeyCommand keyCommandWithInput:@"4"   modifierFlags:0 action:@selector(key4Pressed)],
              [UIKeyCommand keyCommandWithInput:@"'"   modifierFlags:0 action:@selector(key4Pressed)],
              
              [UIKeyCommand keyCommandWithInput:@"5"   modifierFlags:0 action:@selector(key5Pressed)],
              [UIKeyCommand keyCommandWithInput:@"("   modifierFlags:0 action:@selector(key5Pressed)],
              
              [UIKeyCommand keyCommandWithInput:@"6"   modifierFlags:0 action:@selector(key6Pressed)],
              [UIKeyCommand keyCommandWithInput:@"§"   modifierFlags:0 action:@selector(key6Pressed)],
              
              [UIKeyCommand keyCommandWithInput:@"7"   modifierFlags:0 action:@selector(key7Pressed)],
              [UIKeyCommand keyCommandWithInput:@"è"   modifierFlags:0 action:@selector(key7Pressed)],
              
              [UIKeyCommand keyCommandWithInput:@"8"   modifierFlags:0 action:@selector(key8Pressed)],
              [UIKeyCommand keyCommandWithInput:@"!"   modifierFlags:0 action:@selector(key8Pressed)],
              
              [UIKeyCommand keyCommandWithInput:@"9"   modifierFlags:0 action:@selector(key9Pressed)],
              [UIKeyCommand keyCommandWithInput:@"ç"   modifierFlags:0 action:@selector(key9Pressed)],
              
              [UIKeyCommand keyCommandWithInput:@"0"   modifierFlags:0 action:@selector(key0Pressed)],
              [UIKeyCommand keyCommandWithInput:@"à"   modifierFlags:0 action:@selector(key0Pressed)],
              
              [UIKeyCommand keyCommandWithInput:@"f"   modifierFlags:0 action:@selector(keyFPressed)],
              [UIKeyCommand keyCommandWithInput:@"\r"   modifierFlags:0 action:@selector(enterPressed)],
              [UIKeyCommand keyCommandWithInput:@" "   modifierFlags:0 action:@selector(spacePressed)]];
}
-(void)key1Pressed {
    [detailViewControllerIphone switchFX:0];
}
-(void)key2Pressed {
    [detailViewControllerIphone switchFX:1];
}
-(void)key3Pressed {
    [detailViewControllerIphone switchFX:2];
}
-(void)key4Pressed {
    [detailViewControllerIphone switchFX:3];
}
-(void)key5Pressed {
    [detailViewControllerIphone switchFX:4];
}
-(void)key6Pressed {
    [detailViewControllerIphone switchFX:5];
}
-(void)key7Pressed {
    [detailViewControllerIphone switchFX:6];
}
-(void)key8Pressed {
    [detailViewControllerIphone switchFX:7];
}
-(void)key9Pressed {
    [detailViewControllerIphone switchFX:8];
}
-(void)key0Pressed {
    [detailViewControllerIphone switchFX:9];
}
-(void)keyFPressed {
    [detailViewControllerIphone oglButtonPushed];
}
- (void)enterPressed{
    [detailViewControllerIphone oglViewSwitchFS];
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
-(void)leftShiftPressed {
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
-(void)rightShiftPressed {
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
    
}
-(void)spacePressed {
    if (detailViewControllerIphone.mPaused) [detailViewControllerIphone playPushed:nil];
    else [detailViewControllerIphone pausePushed:nil];
}


- (void)pressesBegan:(NSSet<UIPress *> *)presses
           withEvent:(UIPressesEvent *)event {
    if (@available(iOS 13.4, *)) {
    for (UIPress *press in presses) {
        UIKey *key=press.key;
        /*        NSLog(@"key: %@ %@",key.characters,key.charactersIgnoringModifiers);
        if ([key.charactersIgnoringModifiers isEqualToString:@" "]) {
            if (detailViewControllerIphone.mPaused) {
                detailViewControllerIphone.mPaused=0;
                [detailViewControllerIphone.mplayer Pause:NO];
            } else {
                detailViewControllerIphone.mPaused=1;
                [detailViewControllerIphone.mplayer Pause:YES];
            }
        }
        if ([key.charactersIgnoringModifiers isEqualToString:@"UIKeyInputLeftArrow"]) {
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
        if ([key.charactersIgnoringModifiers isEqualToString:@"UIKeyInputRightArrow"]) {
            //1st check if there are more subsongs
            if ((detailViewControllerIphone.mplayer.mod_subsongs>1)&&
                (detailViewControllerIphone.mplayer.mod_currentsub<detailViewControllerIphone.mplayer.mod_maxsub)&&(detailViewControllerIphone.mOnlyCurrentSubEntry==0))
                [detailViewControllerIphone playNextSub];
            else {
                //no more subsongs, check if within an archive to play next entry
                
                if ([detailViewControllerIphone.mplayer isArchive]&&([detailViewControllerIphone.mplayer getArcEntriesCnt]>1)&&([detailViewControllerIphone.mplayer getArcIndex]<[detailViewControllerIphone.mplayer getArcEntriesCnt]-1)&&(detailViewControllerIphone.mOnlyCurrentEntry==0)) {
                    [detailViewControllerIphone.mplayer selectNextArcEntry];
                    [detailViewControllerIphone play_loadArchiveModule];
                } else [detailViewControllerIphone play_nextEntry];
            }
        }*/
    }
    }
    [super pressesBegan:presses withEvent:event];
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
