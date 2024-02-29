//
//  SONFillAnimation.h
//  SONFillAnimation
//
//  Created by Sahnghyun Cha on 13. 7. 9..
//  Copyright (c) 2013 Soncode(Sahn Cha). All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum {
    SONFillAnimationTypeDefault,
    SONFillAnimationTypeFoldOut,
    SONFillAnimationTypeFoldIn,
    SONFillAnimationTypeCustom
} SONFillAnimationType;

typedef enum {
    SONFillAnimationDirectionLeftToRightAndTopDown,
    SONFillAnimationDirectionLeftToRightAndBottomUp,
    SONFillAnimationDirectionRightToLeftAndTopDown,
    SONFillAnimationDirectionRightToLeftAndBottomUp,
    SONFillAnimationDirectionRandom
} SONFillAnimationDirection;

@interface SONFillAnimation : NSObject

/*! A dedicated initializer for SONFillAnimation.
 * \param rows Number of rows
 * \param cols Number of columns
 * \param type Animation type
 * \returns SONFillAnimation object
 */
- (id)initWithNumberOfRows:(NSInteger)rows columns:(NSInteger)cols animationType:(SONFillAnimationType)type;

/*! A rotation vector for FoldIn/FoldOut animation. There's no need to set this vector if the type is Custom.
 * \param x x value
 * \param y y value
 * \param z z value
 */
- (void)setRotationVector:(CGFloat)x :(CGFloat)y :(CGFloat)z;

/*! Begin animation. Note that if animateToHide property is set to YES, view.alpha will be set to 0.0 after the animation completes.
 * \param view A view to apply animation
 * \param block A block to be executed after the animation completes. The block will always be executed even if the animation has canceled
 */
- (void)animateView:(UIView *)view completion:(void (^)(void))block;

/*! Begin animation with specified amount of delay.
 * \param view A view to apply animation.
 * \param delay Delay before starting animation.
 * \param block A block to be executed after the animation completes. The block will always be executed even if the animation has canceled
 */
- (void)animateView:(UIView *)view delay:(NSTimeInterval)delay completion:(void (^)(void))block;



/*! Number of Y slices. */
@property (nonatomic) NSInteger numberOfRows;       // y slices

/*! Number of X slices. */
@property (nonatomic) NSInteger numberOfColumns;    // x slices

/*! Duration of single slice animation. This value will override the duration of any custom transformAnimation or shadeAnimation. */
@property (nonatomic) NSTimeInterval duration;

/*! Delay between slices in the same row. Default value is 0.2f.*/
@property (nonatomic) NSTimeInterval xDelay;

/*! Delay between slices in the same column. Default value is 0.3f.*/
@property (nonatomic) NSTimeInterval yDelay;

/*! Anchor point for each slice of layer. Default value is (0.5, 0.5).*/
@property (nonatomic) CGPoint anchorPoint;

/*! Default direction is 'Left to Right and Top Down'. */
@property (nonatomic) SONFillAnimationDirection direction;

/*! Default type is identical to FoldOut. */
@property (nonatomic) SONFillAnimationType animationType;

/*! Set this property to YES if the animation type is Custom and if the purpose of the animation is to hide the view. The alpha property of the animated view will become 0.0 after the animation completes. Default value is NO. */
@property (nonatomic) BOOL animateToHide;

/*! Set this property to YES in order to disable shadeAnimation. Default value is No. */
@property (nonatomic) BOOL disableShadeAnimation;

/*! Custom transform animation. */
@property (nonatomic, strong) CAAnimation *transformAnimation;

/*! Custom shade animation. */
@property (nonatomic, strong) CAAnimation *shadeAnimation;

/*! Color to be used in the shade animation. Default color is Black.*/
@property (nonatomic, strong) UIColor *shadeColor;

/*! m34 value of the view's transform matrix. Default value is 1.0/100. */
@property (nonatomic) CGFloat m34;

@end
