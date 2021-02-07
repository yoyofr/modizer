//
//  OBSlider.h
//
//  Created by Ole Begemann on 02.01.11.
//  Copyright 2011 Ole Begemann. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>


@interface OBSlider : UISlider {
    NSArray *scrubbingSpeeds;
    NSArray *scrubbingSpeedChangePositions;    
    CGPoint beganTrackingLocation;	
    float scrubbingSpeed;
    float realPositionValue;
}

- (NSUInteger) indexOfLowerScrubbingSpeed:(NSArray*)scrubbingSpeedPositions forOffset:(CGFloat)verticalOffset;
- (NSArray *) defaultScrubbingSpeeds;
- (NSArray *) defaultScrubbingSpeedChangePositions;

@property (nonatomic, retain) NSArray *scrubbingSpeeds;
@property (nonatomic, retain) NSArray *scrubbingSpeedChangePositions;
//@property (nonatomic, assign, readonly) float scrubbingSpeed;
@property (nonatomic, assign) float scrubbingSpeed;
@property (nonatomic, assign) CGPoint beganTrackingLocation;


@end
