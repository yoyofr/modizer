//
//  ImagesCache.m
//
//  Created by Mihael Isaev on 19.01.13.
//  Copyright (c) 2013 Mihael Isaev, Russia, Samara. All rights reserved.
//

#import <Foundation/Foundation.h>

#define ICTypeReal @"real"
#define ICTypeScaled @"scaled"
#define ICPlaceholderFile @"placeholder.png"

@interface ImagesCache : NSObject
{
    NSCache *imagesReal;
    NSCache *imagesScaled;
    NSCache *imagesScaledRetina;
}

-(id)init;
-(UIImage*)getImageWithURL:(NSString*)url
                    prefix:(NSString*)prefix
                      size:(CGSize)size
            forUIImageView:(UIImageView*)uiimageview;

@end
