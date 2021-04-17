//
//  MiniPlayerView.h
//  modizer
//
//  Created by Yohann Magnien on 13/04/2021.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface MiniPlayerView : UIView {
    bool darkMode;
    CGFloat componentsH[4];
    CGFloat componentsL[4];
    CGFloat componentsM[4];
}

@end

NS_ASSUME_NONNULL_END
