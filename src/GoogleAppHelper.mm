//
//  GoogleAppHelper.m
//  modizer
//
//  Created by yoyofr on 23/10/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "GoogleAppHelper.h"
#import "DBHelper.h"

@implementation GoogleAppHelper

+(void) SendStatistics:(NSString*)fileName path:(NSString*)filePath rating:(int)rating playcount:(int)playcount {
	int collectionType=0;
	NSRange r;
	NSString *strPath=nil;
    
    if ([filePath length]>2) {
        if (([filePath characterAtIndex:0]=='/')&&([filePath characterAtIndex:1]=='/')) {
            strPath=@"";
        }
    }
    
    if (strPath==nil) {
	r=[filePath rangeOfString:@"Documents/MODLAND" options:NSCaseInsensitiveSearch];
	if (r.location!=NSNotFound) {
		strPath=DBHelper::getFullPathFromLocalPath([filePath substringFromIndex:18]);
		if (strPath!=nil) collectionType=1;
		else { //Not found but keep path
			strPath=[filePath substringFromIndex:18];
		}
	} else {
		r.location=NSNotFound;
		r=[filePath rangeOfString:@"Documents/HVSC" options:NSCaseInsensitiveSearch];
		if (r.location!=NSNotFound) {
			collectionType=2;
			strPath=[filePath substringFromIndex:15];
		} else {
            r.location=NSNotFound;
            r=[filePath rangeOfString:@"Documents/ASMA" options:NSCaseInsensitiveSearch];
            if (r.location!=NSNotFound) {
                collectionType=3;
                strPath=[filePath substringFromIndex:15];
            }
        }
	}
    }
	if (strPath==nil) {
		//Remove the Documents/ and keep the rest
		strPath=[filePath substringFromIndex:10];
	}
    NSString *identifier;
    if ([[UIDevice currentDevice] respondsToSelector:@selector(identifierForVendor)]) {
        identifier=[[[UIDevice currentDevice] identifierForVendor] UUIDString];
    } else {
        identifier=@"unknwown";//[[UIDevice currentDevice] uniqueIdentifier];
    }
	NSString *urlString=[NSString stringWithFormat:@"%@/Stats?Name=%@&Path=%@&Rating=%d&Played=%d&UID=%@&Type=%d&VersionMajor=%s&VersionMinor=%s",
						 STATISTICS_URL,
						 [[fileName stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding] stringByReplacingOccurrencesOfString:@"&" withString:@"$$"],
						 [[[strPath stringByAddingPercentEscapesUsingEncoding:NSISOLatin1StringEncoding] stringByReplacingOccurrencesOfString:@"%" withString:@"//"] stringByReplacingOccurrencesOfString:@"&" withString:@"$$"],
						 rating,playcount,identifier,
						 collectionType,
                         VERSION_MAJOR_STR,
                         VERSION_MINOR_STR
						  ];
	NSURL *url=[NSURL URLWithString:urlString];
	//NSLog(@"%@",[url absoluteString]);
	
    AFHTTPSessionManager *manager = [AFHTTPSessionManager manager];
    manager.responseSerializer.acceptableContentTypes=[NSSet setWithObject:@"application/json"];
    [manager GET:urlString parameters:nil headers:nil progress:nil success:^(NSURLSessionDataTask * _Nonnull task, id  _Nullable responseObject) {
        //NSLog(@"JSON: %@", responseObject);
    } failure:^(NSURLSessionDataTask * _Nullable task, NSError * _Nonnull error) {
        NSLog(@"Error: %@", error);
    }];
    [manager invalidateSessionCancelingTasks:NO resetSession:NO];
    
}


@end
