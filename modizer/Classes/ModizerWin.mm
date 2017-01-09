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
