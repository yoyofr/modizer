//
//  SONFillAnimation.m
//  SONFillAnimation
//
//  Created by Sahnghyun Cha on 13. 7. 9..
//  Copyright (c) 2013 Soncode(Sahn Cha). All rights reserved.
//

#import "SONFillAnimation.h"
#import <QuartzCore/QuartzCore.h>

#define RADIAN(x) x*M_PI/180.0f

// * GetSubImage:
// * Return a specified portion of the image
@interface UIImage (getSubImage)

- (UIImage *)getSubImageInRect:(CGRect)rect;

@end

@implementation UIImage (getSubImage)

- (UIImage *)getSubImageInRect:(CGRect)rect
{
    UIGraphicsBeginImageContextWithOptions(rect.size, NO, self.scale);
    [self drawAtPoint:CGPointMake(-rect.origin.x, -rect.origin.y)];
    UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return image;
}

@end

// * Rearrange:
// * Rearrange items in NSMutableArray
@interface NSMutableArray (rearrange)

- (void)rearrangeArrayWithDirectionType:(SONFillAnimationDirection)direction row:(NSInteger)row column:(NSInteger)column;

@end

@implementation NSMutableArray (rearrange)

- (void)rearrangeArrayWithDirectionType:(SONFillAnimationDirection)direction row:(NSInteger)row column:(NSInteger)column
{
    int count = [self count];
    if (count == 0) return;
    
    switch (direction) {
        case SONFillAnimationDirectionRandom:
        {
            for (int i = 0; i < count; i++) {
                int pick = (arc4random() % (count - i)) + i;
                [self exchangeObjectAtIndex:i withObjectAtIndex:pick];
            }
            break;
        }
            
        case SONFillAnimationDirectionRightToLeftAndBottomUp:
        {
            int i = 0;
            int j = count - 1;
            while (i < j) {
                [self exchangeObjectAtIndex:i withObjectAtIndex:j];
                i++;
                j--;
            }
            break;
        }
            
        case SONFillAnimationDirectionRightToLeftAndTopDown:
        {
            int i, j;
            int rowCount = 0;
            while (rowCount < row) {
                i = rowCount * column;
                j = ((rowCount + 1) * column) - 1;
                while (i < j) {
                    [self exchangeObjectAtIndex:i withObjectAtIndex:j];
                    i++;
                    j--;
                }
                rowCount++;
            }
            break;
        }
            
        case SONFillAnimationDirectionLeftToRightAndBottomUp:
        {
            int i, j;
            int colCount = 0;
            while (colCount < column) {
                i = colCount;
                j = colCount + (column * (row - 1));
                while (i < j) {
                    [self exchangeObjectAtIndex:i withObjectAtIndex:j];
                    i = i + column;
                    j = j - column;
                }
                colCount++;
            }
            break;
        }
            
        case SONFillAnimationDirectionLeftToRightAndTopDown:
        default:
            break;
    }
}

@end

// * SONFillAnimation:
// * Main dish
@interface SONFillAnimation ()

- (void)performBlock:(void (^)(void))block afterDelay:(NSTimeInterval)delay;
- (void)animateSingleLayer:(CALayer *)layer;
- (NSTimeInterval)animateAllInView:(UIView *)view;

@property (nonatomic) CGFloat rotationVectorX;
@property (nonatomic) CGFloat rotationVectorY;
@property (nonatomic) CGFloat rotationVectorZ;

@end

@implementation SONFillAnimation

#pragma mark Initializers

- (id)init
{
    return [self initWithNumberOfRows:1 columns:1 animationType:SONFillAnimationTypeDefault];
}

- (id)initWithNumberOfRows:(NSInteger)rows columns:(NSInteger)cols animationType:(SONFillAnimationType)type
{
    self = [super init];
    if (self) {
        self.numberOfRows = rows;
        self.numberOfColumns = cols;
        self.duration = 0.5f;
        self.xDelay = 0.2f;
        self.yDelay = 0.3f;
        self.anchorPoint = CGPointMake(0.5, 0.5);
        self.direction = SONFillAnimationDirectionLeftToRightAndTopDown;
        self.animationType = type;
        if (type == SONFillAnimationTypeFoldIn) self.animateToHide = YES;
        else self.animateToHide = NO;
        self.disableShadeAnimation = NO;
        self.shadeColor = [UIColor blackColor];
        self.m34 = 1.0/100;
        self.rotationVectorX = 0;
        self.rotationVectorY = -1;
        self.rotationVectorZ = 0;
    }
    return self;
}

#pragma mark Properties

- (void)setNumberOfRows:(NSInteger)numberOfRows
{
    if (numberOfRows < 1) _numberOfRows = 1;
    else _numberOfRows = numberOfRows;
}

- (void)setNumberOfColumns:(NSInteger)numberOfColumns
{
    if (numberOfColumns < 1) _numberOfColumns = 1;
    else _numberOfColumns = numberOfColumns;
}

- (void)setAnimationType:(SONFillAnimationType)animationType
{
    if (animationType == SONFillAnimationTypeFoldIn) self.animateToHide = YES;
    else if (animationType == SONFillAnimationTypeFoldOut || animationType == SONFillAnimationTypeDefault) self.animateToHide = NO;
    
    _animationType = animationType;
}

#pragma mark Private methods

- (void)performBlock:(void (^)(void))block afterDelay:(NSTimeInterval)delay
{
    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, delay * NSEC_PER_SEC);
    dispatch_after(popTime, dispatch_get_main_queue(), block);
}

- (void)animateSingleLayer:(CALayer *)layer
{
    layer.opacity = 1.0;
    
    CGRect originalFrame = layer.frame;         // Save frame to prevent from repositioning
    layer.anchorPoint = self.anchorPoint;
    layer.frame = originalFrame;
    
    CATransform3D transform = layer.transform;
    transform.m34 = self.m34;
    layer.transform = transform;
    
    CAAnimation *transformAnimation, *shadeAnimation;
    
    switch (self.animationType) {
        case SONFillAnimationTypeCustom:
            transformAnimation = self.transformAnimation;
            shadeAnimation = self.shadeAnimation;
            break;
            
        case SONFillAnimationTypeFoldIn:
        {
            transform = CATransform3DRotate(transform, RADIAN(90), self.rotationVectorX, self.rotationVectorY, self.rotationVectorZ);
            CABasicAnimation *foldInAnimation = [CABasicAnimation animationWithKeyPath:@"transform"];
            foldInAnimation.fromValue = [NSValue valueWithCATransform3D:CATransform3DIdentity];
            foldInAnimation.toValue = [NSValue valueWithCATransform3D:transform];
            foldInAnimation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear];
            
            CABasicAnimation *foldInShadeAnimation = [CABasicAnimation animationWithKeyPath:@"opacity"];
            foldInShadeAnimation.fromValue = [NSNumber numberWithFloat:0.0];
            foldInShadeAnimation.toValue = [NSNumber numberWithFloat:0.5];
            foldInShadeAnimation.timingFunction = foldInAnimation.timingFunction;
            
            transformAnimation = foldInAnimation;
            shadeAnimation = foldInShadeAnimation;
            break;
        } 
            
        case SONFillAnimationTypeFoldOut:       // Fold out = Default
        case SONFillAnimationTypeDefault:
        default:
        {
            transform = CATransform3DRotate(transform, RADIAN(90), self.rotationVectorX, self.rotationVectorY, self.rotationVectorZ);
            CABasicAnimation *foldOutAnimation = [CABasicAnimation animationWithKeyPath:@"transform"];
            foldOutAnimation.fromValue = [NSValue valueWithCATransform3D:transform];
            foldOutAnimation.toValue = [NSValue valueWithCATransform3D:CATransform3DIdentity];
            foldOutAnimation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear];
            
            CABasicAnimation *foldOutShadeAnimation = [CABasicAnimation animationWithKeyPath:@"opacity"];
            foldOutShadeAnimation.fromValue = [NSNumber numberWithFloat:0.5];
            foldOutShadeAnimation.toValue = [NSNumber numberWithFloat:0.0];
            foldOutShadeAnimation.timingFunction = foldOutAnimation.timingFunction;
            
            transformAnimation = foldOutAnimation;
            shadeAnimation = foldOutShadeAnimation;
            break;
        }
    }
    
    if (!transformAnimation) transformAnimation = [CABasicAnimation animation];
    if (!shadeAnimation) shadeAnimation = [CABasicAnimation animation];
    
    transformAnimation.duration = self.duration;
    transformAnimation.fillMode = kCAFillModeForwards;
    transformAnimation.removedOnCompletion = NO;
    shadeAnimation.duration = self.duration;
    shadeAnimation.fillMode = kCAFillModeForwards;
    shadeAnimation.removedOnCompletion = NO;
    
    // Shade layer
    CALayer *shadeLayer = [CALayer layer];
    shadeLayer.frame = CGRectMake(0, 0, layer.frame.size.width, layer.frame.size.height);
    shadeLayer.backgroundColor = self.shadeColor.CGColor;
    shadeLayer.opacity = 0.0;
    [layer addSublayer:shadeLayer];
    
    // Animate layer
    [layer addAnimation:transformAnimation forKey:nil];
    if (!self.disableShadeAnimation) [shadeLayer addAnimation:shadeAnimation forKey:nil];
}

- (NSTimeInterval)animateAllInView:(UIView *)view
{
    // Copy layer to make an image
    view.alpha = 1.0;
    UIGraphicsBeginImageContextWithOptions(view.frame.size, NO, 0.0f);
    [view.layer renderInContext:UIGraphicsGetCurrentContext()];
    
    UIImage *layerImage = UIGraphicsGetImageFromCurrentImageContext();
    CGSize imageSize = layerImage.size;
    UIGraphicsEndImageContext();
    
    view.alpha = 0.0;
    UIView *overlayView = [[UIView alloc] initWithFrame:view.frame];
    overlayView.backgroundColor = [UIColor clearColor];
    [view.superview addSubview:overlayView];
    
    NSMutableArray *subImageViews = [[NSMutableArray alloc] initWithCapacity:self.numberOfColumns * self.numberOfRows];
    
    for (int y = 0; y < self.numberOfRows; y++) {
        for (int x = 0; x < self.numberOfColumns; x++) {
            CGRect partFrame = CGRectMake((imageSize.width / self.numberOfColumns) * x, (imageSize.height / self.numberOfRows) * y, (imageSize.width / self.numberOfColumns), (imageSize.height / self.numberOfRows));
            
            UIImage *subImage = [layerImage getSubImageInRect:partFrame];
            UIImageView *subImageView = [[UIImageView alloc] initWithImage:subImage];
            subImageView.frame = partFrame;
            if (self.animateToHide) subImageView.layer.opacity = 1.0;
            else subImageView.layer.opacity = 0.0;
            
            [overlayView addSubview:subImageView];
            [subImageViews addObject:subImageView];
        }
    }
    
    double totalDelay = 0;
    int objectCount = 0;
    
    [subImageViews rearrangeArrayWithDirectionType:self.direction row:self.numberOfRows column:self.numberOfColumns];
    
    for (UIImageView *aView in subImageViews) {
        totalDelay = ((objectCount % self.numberOfColumns) * self.xDelay) + ((objectCount / self.numberOfColumns) * self.yDelay);
        [self performSelector:@selector(animateSingleLayer:) withObject:aView.layer afterDelay:totalDelay];
        objectCount++;
    }
    
    [self performBlock:^{
        [overlayView removeFromSuperview];
        if (self.animateToHide) view.alpha = 0.0;
        else view.alpha = 1.0;
    } afterDelay:totalDelay + self.duration];
    
    return totalDelay + self.duration;
}


#pragma mark Public methods

- (void)setRotationVector:(CGFloat)x :(CGFloat)y :(CGFloat)z
{
    self.rotationVectorX = x;
    self.rotationVectorY = y;
    self.rotationVectorZ = z;
    
    if (self.rotationVectorX > 1.0) self.rotationVectorX = 1.0;
    else if (self.rotationVectorX < -1.0) self.rotationVectorX = -1.0;
    if (self.rotationVectorY > 1.0) self.rotationVectorY = 1.0;
    else if (self.rotationVectorY < -1.0) self.rotationVectorY = -1.0;
    if (self.rotationVectorZ > 1.0) self.rotationVectorZ = 1.0;
    else if (self.rotationVectorZ < -1.0) self.rotationVectorZ = -1.0;
}

- (void)animateView:(UIView *)view completion:(void (^)(void))block
{
    NSTimeInterval completionDelay;
    completionDelay = [self animateAllInView:view];
    [self performBlock:block afterDelay:completionDelay];
}

- (void)animateView:(UIView *)view delay:(NSTimeInterval)delay completion:(void (^)(void))block
{
    [self performBlock:^{
        [self animateView:view completion:block];
    } afterDelay:delay];
}

@end
