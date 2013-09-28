//
//  MDZUIImageView.m
//  modizer
//
//  Created by Yohann Magnien on 27/09/13.
//
//

#import "MDZUIImageView.h"

@implementation MDZUIImageView

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code
    }
    return self;
}

- (void)setFrame:(CGRect)frame
{
    if ([[[UIDevice currentDevice] systemVersion] compare:@"7.0" options:NSNumericSearch] != NSOrderedAscending) {
        // background view covers delete button on iOS 7 !?!
        [super setFrame:CGRectMake(0, frame.origin.y, frame.size.width, frame.size.height)];
    } else {
        [super setFrame:frame];
    }
}

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect
{
    // Drawing code
}
*/

@end
