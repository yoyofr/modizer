//
//  CustomSlider.m
//  Measures
//
//  Created by Michael Neuwert on 4/26/11.
//  Copyright 2011 Neuwert Media. All rights reserved.
//

#import "MNEValueTrackingSlider.h"

#pragma mark - Private UIView subclass rendering the popup showing slider value

@interface MNESliderValuePopupView : UIView  
@property (nonatomic) float value;
@property (nonatomic, retain) UIFont *font;
@property (nonatomic, retain) NSString *text;
@end

@implementation MNESliderValuePopupView

@synthesize value=_value;
@synthesize font=_font;
@synthesize text = _text;


- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.font = [UIFont boldSystemFontOfSize:10];
    }
    return self;
}

- (void)dealloc {
    self.text = nil;
    self.font = nil;
    [super dealloc];
}

- (void)drawRect:(CGRect)rect {
    
    // Set the fill color
	[[UIColor colorWithWhite:0 alpha:0.8] setFill];

    // Create the path for the rounded rectangle
    CGRect roundedRect = CGRectMake(self.bounds.origin.x, self.bounds.origin.y, self.bounds.size.width, floorf(self.bounds.size.height * 0.8));
    UIBezierPath *roundedRectPath = [UIBezierPath bezierPathWithRoundedRect:roundedRect cornerRadius:6.0];
    
    // Create the arrow path
    UIBezierPath *arrowPath = [UIBezierPath bezierPath];
    CGFloat maxX =  CGRectGetMaxX(self.bounds);
    CGPoint p0 = CGPointMake(maxX, CGRectGetMaxY(self.bounds));
    [arrowPath moveToPoint:p0];
    [arrowPath addLineToPoint:CGPointMake((maxX - 10.0), CGRectGetMaxY(roundedRect))];
    [arrowPath addLineToPoint:CGPointMake((maxX + 10.0), CGRectGetMaxY(roundedRect))];
    [arrowPath closePath];
    
    // Attach the arrow path to the rounded rect
    [roundedRectPath appendPath:arrowPath];

    [roundedRectPath fill];

    // Draw the text
    if (self.text) {
        [[UIColor colorWithWhite:1 alpha:0.8] set];
        CGSize s = [_text sizeWithFont:self.font];
        CGFloat yOffset = (roundedRect.size.height - s.height) / 2;
        CGRect textRect = CGRectMake(roundedRect.origin.x, yOffset, roundedRect.size.width, s.height);
        
        [_text drawInRect:textRect 
                 withFont:self.font 
            lineBreakMode:UILineBreakModeWordWrap 
                alignment:UITextAlignmentCenter];    
    }
}

- (void)setValue:(float)aValue {
    _value = aValue;
    if (ceil(_value)==_value) self.text = [NSString stringWithFormat:@"%4.0f", _value];
    else self.text = [NSString stringWithFormat:@"%4.2f", _value];
    [self setNeedsDisplay];
}

- (void)setValue:(float)aValue sValue:(NSString *)sValue {
    _value = aValue;
    self.text = [NSString stringWithString:sValue];
    [self setNeedsDisplay];
}


@end

#pragma mark - MNEValueTrackingSlider implementations

@implementation MNEValueTrackingSlider

@synthesize thumbRect;
@synthesize integerMode;

#pragma mark - Private methods

- (void)setValue:(float)aValue sValue:(NSString *)sValue {
    [valuePopupView setValue:aValue sValue:sValue];    
}


- (void)_constructSlider {
    valuePopupView = [[MNESliderValuePopupView alloc] initWithFrame:CGRectZero];
    valuePopupView.backgroundColor = [UIColor clearColor];
    valuePopupView.alpha = 0.0;
    integerMode=0;
    [self addSubview:valuePopupView];    
}

- (void)_fadePopupViewInAndOut:(BOOL)aFadeIn {
    [UIView beginAnimations:nil context:NULL];
    [UIView setAnimationDuration:1.0];
    if (aFadeIn) {
        valuePopupView.alpha = 1.0;
    } else {
        valuePopupView.alpha = 0.0;
    }
    [UIView commitAnimations];
}

- (void)_positionAndUpdatePopupView {
    CGRect _thumbRect = self.thumbRect;
    CGRect popupRect = CGRectOffset(_thumbRect, -50, 10-floorf(_thumbRect.size.height * 0.8));
    valuePopupView.frame = CGRectInset(popupRect, -20, 2);
    switch (integerMode) {
        case 0://float
            valuePopupView.value = self.value;
            break;
        case 1://int
            valuePopupView.value = (NSInteger)self.value;;
            break;
        case 2://time
            [valuePopupView setValue:self.value sValue:[NSString stringWithFormat:@"%02d:%02d",(NSInteger)(self.value/60),(NSInteger)(self.value)%60]];
            break;
    }
}

#pragma mark - Memory management

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self _constructSlider];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if (self) {
        [self _constructSlider];
    }
    return self;
}

- (void)dealloc {
    [valuePopupView release];
    [super dealloc];
}

#pragma mark - UIControl touch event tracking

- (BOOL)beginTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event {
    // Fade in and update the popup view
    CGPoint touchPoint = [touch locationInView:self];
    // Check if the knob is touched. Only in this case show the popup-view
    if(CGRectContainsPoint(CGRectInset(self.thumbRect, -12.0, -12.0), touchPoint)) {
        [self _positionAndUpdatePopupView];
        [self _fadePopupViewInAndOut:YES]; 
    }
    return [super beginTrackingWithTouch:touch withEvent:event];
}

- (BOOL)continueTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event {
    // Update the popup view as slider knob is being moved
    [self _positionAndUpdatePopupView];
    return [super continueTrackingWithTouch:touch withEvent:event];
}

- (void)cancelTrackingWithEvent:(UIEvent *)event {
    [self _positionAndUpdatePopupView];
    [super cancelTrackingWithEvent:event];
}

- (void)endTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event {
    // Fade out the popoup view
    [self _positionAndUpdatePopupView];
    [self _fadePopupViewInAndOut:NO];
    [super endTrackingWithTouch:touch withEvent:event];
}

#pragma mark - Custom property accessors

- (CGRect)thumbRect {
    CGRect trackRect = [self trackRectForBounds:self.bounds];
    CGRect thumbR = [self thumbRectForBounds:self.bounds 
                                         trackRect:trackRect
                                             value:self.value];
    return thumbR;
}

@end
