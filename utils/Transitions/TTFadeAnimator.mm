//
//  TTFadeAnimator.m
//  TransitionTutorial
//
//  Created by Brad Bambara on 4/14/14.
//  Copyright (c) 2014 Brad Bambara. All rights reserved.
//

#import "TTFadeAnimator.h"

#define kTransitionDuration 0.35

@implementation TTFadeAnimator

#pragma mark - UIViewControllerAnimatedTransitioning

- (NSTimeInterval)transitionDuration:(id <UIViewControllerContextTransitioning>)transitionContext
{
	return kTransitionDuration;
}

- (void)animateTransition:(id <UIViewControllerContextTransitioning>)transitionContext
{
	//fade the new view in
	UIViewController* toController = [transitionContext viewControllerForKey:UITransitionContextToViewControllerKey];
	UIViewController* fromController = [transitionContext viewControllerForKey:UITransitionContextFromViewControllerKey];
	UIView* container = [transitionContext containerView];
	
	toController.view.alpha = 0.0f;
	[container addSubview:toController.view];
	
	[UIView animateWithDuration:kTransitionDuration animations:^{
		toController.view.alpha = 1.0f;
	} completion:^(BOOL finished){
		[fromController.view removeFromSuperview];
		[transitionContext completeTransition:finished];
	}];
}

@end
