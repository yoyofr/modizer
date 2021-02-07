//
//  ASIFormDataRequest.h
//  Part of ASIHTTPRequest -> http://allseeing-i.com/ASIHTTPRequest
//
//  Created by Ben Copsey on 07/11/2008.
//  Copyright 2008-2009 All-Seeing Interactive. All rights reserved.
//


//  Modifyed for ARC by Jayesh on 19/06/14.
//  Copyright (c) 2012 jayeshkavathiya@gmail.com.

#import <Foundation/Foundation.h>
#import "ASIHTTPRequest.h"
#import "ASIHTTPRequestConfig.h"

typedef enum _ASIPostFormat {
    ASIMultipartFormDataPostFormat = 0,
    ASIURLEncodedPostFormat = 1
	
} ASIPostFormat;

@interface ASIFormDataRequest : ASIHTTPRequest <NSCopying> {

	// Parameters that will be POSTed to the url
	NSMutableDictionary *postData;
	
	// Files that will be POSTed to the url
	NSMutableDictionary *fileData;
	
	ASIPostFormat postFormat;
	
	NSStringEncoding stringEncoding;
	
#if DEBUG_FORM_DATA_REQUEST
	// Will store a string version of the request body that will be printed to the console when ASIHTTPREQUEST_DEBUG is set in GCC_PREPROCESSOR_DEFINITIONS
	NSString *debugBodyString;
#endif
	
}

#pragma mark utilities 
- (NSString*)encodeURL:(NSString *)string; 
 
#pragma mark setup request

// Add a POST variable to the request
- (void)setPostValue:(id <NSObject>)value forKey:(NSString *)key;

// Add the contents of a local file to the request
- (void)setFile:(NSString *)filePath forKey:(NSString *)key;

// Same as above, but you can specify the content-type and file name
- (void)setFile:(id)data withFileName:(NSString *)fileName andContentType:(NSString *)contentType forKey:(NSString *)key;

// Add the contents of an NSData object to the request
- (void)setData:(NSData *)data forKey:(NSString *)key;

// Same as above, but you can specify the content-type and file name
- (void)setData:(id)data withFileName:(NSString *)fileName andContentType:(NSString *)contentType forKey:(NSString *)key;


@property (assign) ASIPostFormat postFormat;
@property (assign) NSStringEncoding stringEncoding;
@end
