//
//  MiniPlayerView.m
//  modizer
//
//  Created by Yohann Magnien on 13/04/2021.
//
#import "ModizerConstants.h"
#import "MiniPlayerView.h"

@implementation MiniPlayerView

- (instancetype)init
{
    self = [super init];
    if (self) {
        darkMode=false;
        if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
            if (@available(iOS 12.0, *)) {
                if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
            }
        }
        
    }
    
    return self;
}

- (void)layoutSubviews {    
    [super layoutSubviews];
    [self setNeedsDisplay];
}


- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
}

// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSetLineWidth(context, 2.0);
    CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
    
    if (darkMode) {
        componentsH[0] = 0.15;
        componentsH[1] = 0.15;
        componentsH[2] = 0.15;
        componentsH[3] = 1.0;
        componentsL[0] = 0.05;
        componentsL[1] = 0.05;
        componentsL[2] = 0.05;
        componentsL[3] = 1.0;
        componentsM[0] = 0.2;
        componentsM[1] = 0.2;
        componentsM[2] = 0.2;
        componentsM[3] = 1.0;
    } else {
        componentsH[0] = 0.95;
        componentsH[1] = 0.95;
        componentsH[2] = 0.95;
        componentsH[3] = 1.0;
        componentsL[0] = 0.85;
        componentsL[1] = 0.85;
        componentsL[2] = 0.85;
        componentsL[3] = 1.0;
        componentsM[0] = 0.85;
        componentsM[1] = 0.85;
        componentsM[2] = 0.85;
        componentsM[3] = 1.0;
    }
    CGColorRef color = CGColorCreate(colorspace, componentsH);
    CGContextSetStrokeColorWithColor(context, color);
    CGContextMoveToPoint(context, self.frame.origin.x, self.frame.origin.y+0);
    CGContextAddLineToPoint(context, self.frame.size.width, self.frame.origin.y+0);
        
    CGContextStrokePath(context);
    
    
    CGColorRelease(color);

    color = CGColorCreate(colorspace, componentsL);
    CGContextSetStrokeColorWithColor(context, color);
    CGContextMoveToPoint(context, self.frame.origin.x, self.frame.origin.y+self.frame.size.height-1);
    CGContextAddLineToPoint(context, self.frame.size.width, self.frame.origin.y+self.frame.size.height-1);
    CGContextStrokePath(context);
    CGColorRelease(color);
    
    CGContextSetLineWidth(context, 1.0);
    color = CGColorCreate(colorspace, componentsM);
    CGContextSetStrokeColorWithColor(context, color);
    CGContextMoveToPoint(context, self.frame.size.width-50, self.frame.origin.y+24-20);
    CGContextAddLineToPoint(context, self.frame.size.width-50, self.frame.origin.y+24+20);
    CGContextMoveToPoint(context, self.frame.size.width-100, self.frame.origin.y+24-20);
    CGContextAddLineToPoint(context, self.frame.size.width-100, self.frame.origin.y+24+20);
    CGContextMoveToPoint(context, self.frame.size.width-150, self.frame.origin.y+24-20);
    CGContextAddLineToPoint(context, self.frame.size.width-150, self.frame.origin.y+24+20);
    CGContextStrokePath(context);
    
    CGColorRelease(color);
    
    CGColorSpaceRelease(colorspace);
    
    
    
}


@end
