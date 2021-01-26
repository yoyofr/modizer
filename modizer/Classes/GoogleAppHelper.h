//
//  GoogleAppHelper.h
//  modizer
//
//  Created by yoyofr on 23/10/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "AFNetworking.h"
#import "AFHTTPSessionManager.h"
#import "ModizerConstants.h"

@interface GoogleAppHelper : NSObject {

}

+(void) SendStatistics:(NSString*)fileName path:(NSString*)filePath rating:(int)rating playcount:(int)playcount country:(NSString*)country city:(NSString*)city longitude:(double)lon latitude:(double)lat;

@end
