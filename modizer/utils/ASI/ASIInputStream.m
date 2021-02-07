//
//  ASIInputStream.m
//  Part of ASIHTTPRequest -> http://allseeing-i.com/ASIHTTPRequest
//
//  Created by Ben Copsey on 10/08/2009.
//  Copyright 2009 All-Seeing Interactive. All rights reserved.
//


//  Modifyed for ARC by Jayesh on 19/06/14.
//  Copyright (c) 2012 jayeshkavathiya@gmail.com.

#import "ASIInputStream.h"
#import "ASIHTTPRequest.h"

// Used to ensure only one request can read data at once
static NSLock *readLock = nil;

@implementation ASIInputStream

+ (void)initialize
{
	if (self == [ASIInputStream class]) {
		readLock = [[NSLock alloc] init];
	}
}

+ (id)inputStreamWithFileAtPath:(NSString *)path request:(ASIHTTPRequest *)request
{
	ASIInputStream *stream = [[self alloc] init] ;
	[stream setRequest:request];
	[stream setStream:[NSInputStream inputStreamWithFileAtPath:path]];
	return stream;
}

+ (id)inputStreamWithData:(NSData *)data request:(ASIHTTPRequest *)request
{
	ASIInputStream *stream = [[self alloc] init];
	[stream setRequest:request];
	[stream setStream:[NSInputStream inputStreamWithData:data]];
	return stream;
}

// Called when CFNetwork wants to read more of our request body
// When throttling is on, we ask ASIHTTPRequest for the maximum amount of data we can read
- (NSInteger)read:(uint8_t *)buffer maxLength:(NSUInteger)len
{
	[readLock lock];
	unsigned long toRead = len;
	if ([ASIHTTPRequest isBandwidthThrottled]) {
		toRead = [ASIHTTPRequest maxUploadReadLength];
		if (toRead > len) {
			toRead = len;
		} else if (toRead == 0) {
			toRead = 1;
		}
		[request performThrottling];
	}
	[ASIHTTPRequest incrementBandwidthUsedInLastSecond:toRead];
	[readLock unlock];
	return [stream read:buffer maxLength:toRead];
}

/*
 * Implement NSInputStream mandatory methods to make sure they are implemented
 * (necessary for MacRuby for example) and avoir the overhead of method
 * forwarding for these common methods.
 */
- (void)open
{
    [stream open];
}

- (void)close
{
    [stream close];
}

- (id)delegate
{
    return [stream delegate];
}

- (void)setDelegate:(id)delegate
{
    [stream setDelegate:delegate];
}

- (void)scheduleInRunLoop:(NSRunLoop *)aRunLoop forMode:(NSString *)mode
{
    [stream scheduleInRunLoop:aRunLoop forMode:mode];
}

- (void)removeFromRunLoop:(NSRunLoop *)aRunLoop forMode:(NSString *)mode
{
    [stream removeFromRunLoop:aRunLoop forMode:mode];
}

- (id)propertyForKey:(NSString *)key
{
    return [stream propertyForKey:key];
}

- (BOOL)setProperty:(id)property forKey:(NSString *)key
{
    return [stream setProperty:property forKey:key];
}

- (NSStreamStatus)streamStatus
{
    return [stream streamStatus];
}

- (NSError *)streamError
{
    return [stream streamError];
}

// If we get asked to perform a method we don't have (probably internal ones),
// we'll just forward the message to our stream

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
	return [stream methodSignatureForSelector:aSelector];
}
	 
- (void)forwardInvocation:(NSInvocation *)anInvocation
{
	[anInvocation invokeWithTarget:stream];
}

@synthesize stream;
@synthesize request;
@end
