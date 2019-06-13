//
//  EQView.m
//  modizer
//
//  Created by Yohann Magnien on 08/09/13.
//  
//

#import "EQView.h"

extern BOOL is_ios7;

@implementation EQView

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code
    }
    return self;
}

- (void)drawRect:(CGRect)rect {
    [super drawRect:rect];
    
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSetStrokeColorWithColor(context, [UIColor blackColor].CGColor);
    
    float dashPattern[] = {1,1}; //make your pattern here
    CGContextSetLineDash(context,0, dashPattern,4);
    
    CGContextSetLineWidth(context, 0.5);
    
    int yofs=(is_ios7?16:12);
    CGContextMoveToPoint(context, 4+28,self.frame.size.height/4+32);
    CGContextAddLineToPoint(context, self.frame.size.width-4,self.frame.size.height/4+32);
    
    CGContextMoveToPoint(context, 4+28,self.frame.size.height/2+32-yofs);
    CGContextAddLineToPoint(context, self.frame.size.width-4,self.frame.size.height/2+32-yofs);
    
    CGContextMoveToPoint(context, 4+28,32+yofs);
    CGContextAddLineToPoint(context, self.frame.size.width-4,32+yofs);
    // and now draw the Path!
    CGContextStrokePath(context);
}


@end
