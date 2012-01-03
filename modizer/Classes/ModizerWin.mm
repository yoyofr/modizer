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
		if ((detailViewControllerIphone.mplayer.mod_subsongs>1)&&
			(detailViewControllerIphone.mplayer.mod_currentsub<detailViewControllerIphone.mplayer.mod_maxsub)) 
			[detailViewControllerIphone playNextSub]; //should handle sub ?
		else [detailViewControllerIphone playNext];
	}
	if (event.subtype == UIEventSubtypeRemoteControlPreviousTrack) {
		if ((detailViewControllerIphone.mplayer.mod_subsongs>1)&&
			(detailViewControllerIphone.mplayer.mod_currentsub>detailViewControllerIphone.mplayer.mod_minsub)) 
			[detailViewControllerIphone playPrevSub]; //should handle sub ?
		else [detailViewControllerIphone playPrev];
	}
}


- (BOOL)canBecomeFirstResponder {
	return YES;
}

@end
