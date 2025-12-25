//
//  C64PGGDecoder.h
//  modizer
//
//  Created by Yohann Magnien David on 25/12/2025.
//

#import <UIKit/UIKit.h>

@interface C64PGGDecoder : NSObject

+ (UIImage *)imageFromPGGData:(NSData *)data;

@end
