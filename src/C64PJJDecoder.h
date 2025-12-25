//
//  C64PJJDecoder.h
//  modizer
//
//  Created by Yohann Magnien David on 25/12/2025.
//

#import <UIKit/UIKit.h>

@interface C64PJJDecoder : NSObject

+ (UIImage *)imageFromPJJData:(NSData *)data;

@end
