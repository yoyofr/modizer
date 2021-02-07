//
//  ASINSStringAdditions.m
//  Part of ASIHTTPRequest -> http://allseeing-i.com/ASIHTTPRequest
//
//  Created by Ben Copsey on 12/09/2008.
//  Copyright 2008 All-Seeing Interactive. All rights reserved.
//


//  Modifyed for ARC by Jayesh on 19/06/14.
//  Copyright (c) 2012 jayeshkavathiya@gmail.com.

#import "ASINSStringAdditions.h"

@implementation NSString (CookieValueEncodingAdditions)

- (NSString *)decodedCookieValue
{
	NSMutableString *s = [NSMutableString stringWithString:[self stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
	//Also swap plus signs for spaces
	[s replaceOccurrencesOfString:@"+" withString:@" " options:NSLiteralSearch range:NSMakeRange(0, [s length])];
	return [NSString stringWithString:s];
}

- (NSString *)encodedCookieValue
{
	return [self stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
}

@end


