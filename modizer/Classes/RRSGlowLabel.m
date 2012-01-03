//
//  RRSGlowLabel.m
//  TextGlowDemo
//
//  Created by Andrew on 28/04/2010.
//  Red Robot Studios 2010.
//

#import "RRSGlowLabel.h"


@implementation RRSGlowLabel

@synthesize glowColor, glowOffset, glowAmount;

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if(self != nil) {
        self.glowOffset = CGSizeMake(0.0, 0.0);
        self.glowAmount = 0.0;
        self.glowColor = [UIColor clearColor];
		
    }
    return self;
}

- (void)drawTextInRect:(CGRect)rect {
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSaveGState(context);
    
    CGContextSetShadow(context, self.glowOffset, self.glowAmount);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    
    CGColorRef color = CGColorCreate(colorSpace, CGColorGetComponents(self.glowColor.CGColor));
    CGContextSetShadowWithColor(context, self.glowOffset, self.glowAmount, color);
    
    [super drawTextInRect:rect];
    
    CGColorRelease(color);
    CGColorSpaceRelease(colorSpace);
    CGContextRestoreGState(context);
}

- (void)dealloc {
    [glowColor release];
    self.glowColor = nil;
    [super dealloc];
}

@end
