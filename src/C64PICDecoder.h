//
//  C64PICDecoder.h
//  modizer
//
//  Created by Yohann Magnien David on 25/12/2025.
//

#import <UIKit/UIKit.h>

@interface C64PICDecoder : NSObject

+ (UIImage *)imageFromPICData:(NSData *)data;

@end
