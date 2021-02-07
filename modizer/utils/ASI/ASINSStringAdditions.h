//
//  ASINSStringAdditions.h
//  Part of ASIHTTPRequest -> http://allseeing-i.com/ASIHTTPRequest
//
//  Created by Ben Copsey on 12/09/2008.
//  Copyright 2008 All-Seeing Interactive. All rights reserved.
//


//  Modifyed for ARC by Jayesh on 19/06/14.
//  Copyright (c) 2012 jayeshkavathiya@gmail.com.

#import <Foundation/Foundation.h>

@interface NSString (CookieValueEncodingAdditions)

- (NSString *)encodedCookieValue;
- (NSString *)decodedCookieValue;

@end
