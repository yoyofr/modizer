//
//  CoverScrapper.h
//  modizer
//
//  Created by Yohann Magnien David on 29/01/2026.
//

#import <Foundation/Foundation.h>
#import "StringMatcher.h"

@interface CoverScrapper : NSObject

- (void)getImgfromImgGrabber:(NSString*)grabber_url search_label:(NSString*)search_label label:(NSString*)label fullpath:(NSString*)fullpath completion:(void (^)(void))block;

@property (nonatomic, strong) StringMatcher *matcher;
    
@end
