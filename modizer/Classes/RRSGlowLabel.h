//
//  RRSGlowLabel.h
//  TextGlowDemo
//
//  Created by Andrew on 28/04/2010.
//  Copyright 2010 Red Robot Studios. All rights reserved.
//


@interface RRSGlowLabel : UILabel {
    CGSize glowOffset;
    UIColor *glowColor;
    CGFloat glowAmount;
}

@property (nonatomic, assign) CGSize glowOffset;
@property (nonatomic, assign) CGFloat glowAmount;
@property (nonatomic, retain) UIColor *glowColor;

@end
