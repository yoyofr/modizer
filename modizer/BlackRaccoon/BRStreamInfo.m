//----------
//
//				BRStreamInfo.m
//
// filename:	BRStreamInfo.m
//
// author:      Created by Lloyd Sargent for ARC compliance.
//              Copyright 2012 Lloyd Sargent. All rights reserved.
//
// created:		Jul 04, 2012
//
// description:	
//
// notes:		none
//
// revisions:	
//
// license:     Permission is hereby granted, free of charge, to any person obtaining a copy
//              of this software and associated documentation files (the "Software"), to deal
//              in the Software without restriction, including without limitation the rights
//              to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//              copies of the Software, and to permit persons to whom the Software is
//              furnished to do so, subject to the following conditions:
//
//              The above copyright notice and this permission notice shall be included in
//              all copies or substantial portions of the Software.
//
//              THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//              IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//              FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//              AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//              LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//              OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//              THE SOFTWARE.
//



//---------- pragmas



//---------- include files
#import "BRStreamInfo.h"
//#import "memory.h"
#import "BRRequest.h"



//---------- enumerated data types



//---------- typedefs



//---------- definitions



//---------- structs



//---------- external functions



//---------- external variables



//---------- global functions



//---------- local functions



//---------- global variables



//---------- local variables



//---------- protocols



//---------- classes

@implementation BRStreamInfo

@synthesize writeStream;    
@synthesize readStream;
@synthesize bytesThisIteration;
@synthesize bytesTotal;
@synthesize timeout;
@synthesize cancelRequestFlag;
@synthesize cancelDoesNotCallDelegate;


//-----
//
//              dispatch_get_local_queue
//
// synopsis:	retval = dispatch_get_local_queue();
//					 retval	-
//
// description:	dispatch_get_local_queue() is designed to get our local queue,
//              if it exists, or create one if it doesn't exist.
//
// errors:		none
//
// returns:		queue of type dispatch_queue_t
//


dispatch_queue_t dispatch_get_local_queue()
{
    static dispatch_queue_t _queue;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _queue = dispatch_queue_create("com.github.blackraccoon", 0);
        dispatch_queue_set_specific(_queue, "com.github.blackraccoon", (void*) "com.github.blackraccoon", NULL);
    });
    return _queue;
}



//-----
//
//				openRead
//
// synopsis:	[self openRead:request];
//					BRRequest *request	-
//
// description:	openRead is designed to
//
// errors:		none
//
// returns:		none
//

- (void)openRead:(BRRequest *)request
{
    if (request.hostname==nil)
    {
        InfoLog(@"The host name is nil!");
        request.error = [[BRRequestError alloc] init];
        request.error.errorCode = kBRFTPClientHostnameIsNil;
        [request.delegate requestFailedBR: request];
        [request.streamInfo close: request];
        return;
    }
    
    // a little bit of C because I was not able to make NSInputStream play nice
    CFReadStreamRef readStreamRef = CFReadStreamCreateWithFTPURL(NULL, ( __bridge CFURLRef) request.fullURL);
    
    CFReadStreamSetProperty(readStreamRef, kCFStreamPropertyShouldCloseNativeSocket, kCFBooleanTrue);
	CFReadStreamSetProperty(readStreamRef, kCFStreamPropertyFTPUsePassiveMode, request.passiveMode ? kCFBooleanTrue : kCFBooleanFalse);
    CFReadStreamSetProperty(readStreamRef, kCFStreamPropertyFTPFetchResourceInfo, kCFBooleanTrue);
    CFReadStreamSetProperty(readStreamRef, kCFStreamPropertyFTPUserName, (__bridge CFStringRef) request.username);
    CFReadStreamSetProperty(readStreamRef, kCFStreamPropertyFTPPassword, (__bridge CFStringRef) request.password);
    readStream = ( __bridge_transfer NSInputStream *) readStreamRef;
    
    if (readStream==nil)
    {
        InfoLog(@"Can't open the read stream! Possibly wrong URL");
        request.error = [[BRRequestError alloc] init];
        request.error.errorCode = kBRFTPClientCantOpenStream;
        [request.delegate requestFailedBR: request];
        [request.streamInfo close: request];
        return;
    }
    
    readStream.delegate = request;
	[readStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
	[readStream open];
    
    request.didOpenStream = NO;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, timeout * NSEC_PER_SEC), dispatch_get_local_queue(), ^{
        if (!request.didOpenStream && request.error == nil)
        {
            InfoLog(@"No response from the server. Timeout.");
            request.error = [[BRRequestError alloc] init];
            request.error.errorCode = kBRFTPClientStreamTimedOut;
            [request.delegate requestFailedBR: request];
            [request.streamInfo close: request];
        }
    });
}



//-----
//
//				openWrite
//
// synopsis:	[self openWrite:request];
//					BRRequest *request	-
//
// description:	openWrite is designed to
//
// errors:		none
//
// returns:		none
//

- (void)openWrite:(BRRequest *)request
{
    if (request.hostname==nil)
    {
        InfoLog(@"The host name is nil!");
        request.error = [[BRRequestError alloc] init];
        request.error.errorCode = kBRFTPClientHostnameIsNil;
        [request.delegate requestFailedBR: request];
        [request.streamInfo close: request];
        return;
    }
    
    CFWriteStreamRef writeStreamRef = CFWriteStreamCreateWithFTPURL(NULL, ( __bridge CFURLRef) request.fullURL);
    
    CFWriteStreamSetProperty(writeStreamRef, kCFStreamPropertyShouldCloseNativeSocket, kCFBooleanTrue);
	CFWriteStreamSetProperty(writeStreamRef, kCFStreamPropertyFTPUsePassiveMode, request.passiveMode ? kCFBooleanTrue : kCFBooleanFalse);
    CFWriteStreamSetProperty(writeStreamRef, kCFStreamPropertyFTPFetchResourceInfo, kCFBooleanTrue);
    CFWriteStreamSetProperty(writeStreamRef, kCFStreamPropertyFTPUserName, (__bridge CFStringRef) request.username);
    CFWriteStreamSetProperty(writeStreamRef, kCFStreamPropertyFTPPassword, (__bridge CFStringRef) request.password);
    
    writeStream = ( __bridge_transfer NSOutputStream *) writeStreamRef;
    
    if (writeStream == nil)
    {
        InfoLog(@"Can't open the write stream! Possibly wrong URL!");
        request.error = [[BRRequestError alloc] init];
        request.error.errorCode = kBRFTPClientCantOpenStream;
        [request.delegate requestFailedBR: request];
        [request.streamInfo close: request];
        return;
    }
    
    writeStream.delegate = request;
    [writeStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [writeStream open];
    
    request.didOpenStream = NO;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, timeout * NSEC_PER_SEC), dispatch_get_local_queue(), ^{
        if (!request.didOpenStream && request.error==nil)
        {
            InfoLog(@"No response from the server. Timeout.");
            request.error = [[BRRequestError alloc] init];
            request.error.errorCode = kBRFTPClientStreamTimedOut;
            [request.delegate requestFailedBR:request];
            [request.streamInfo close: request];
        }
    });
}



//-----
//
//				checkCancelRequest
//
// synopsis:	[self checkCancelRequest:request];
//					BRRequest *request	-
//
// description:	checkCancelRequest is designed to
//
// errors:		none
//
// returns:		none
//

- (BOOL)checkCancelRequest:(BRRequest *)request
{
    if (!cancelRequestFlag)
        return NO;
    
    //----- see if we don't want to call the delegate (set and forget)
    if (cancelDoesNotCallDelegate == YES)
    {
        [request.streamInfo close: request];
    }
    
    //----- otherwise indicate that the request to cancel was completed
    else
    {
        [request.delegate requestCompletedBR: request];
        [request.streamInfo close: request];
    }
    
    return YES;
}



//-----
//
//				read
//
// synopsis:	retval = [self read:request];
//					NSData *retval    	-
//					BRRequest *request	-
//
// description:	read is designed to
//
// errors:		none
//
// returns:		Variable of type NSData *
//

- (NSData *)read:(BRRequest *)request
{
    NSData *data;
    NSMutableData *bufferObject = [NSMutableData dataWithLength: kBRDefaultBufferSize];

    bytesThisIteration = [readStream read: (UInt8 *) [bufferObject bytes] maxLength:kBRDefaultBufferSize];
    bytesTotal += bytesThisIteration;
    
    //----- return the data
    if (bytesThisIteration > 0)
    {
        data = [NSData dataWithBytes: (UInt8 *) [bufferObject bytes] length: bytesThisIteration];
        
        request.percentCompleted = bytesTotal / request.maximumSize;
        
        if ([request.delegate respondsToSelector:@selector(percentCompleted:)])
        {
            [request.delegate percentCompleted: request];
        }
        
        return data;
    }
    
    //----- return no data, but this isn't an error... just the end of the file
    else if (bytesThisIteration == 0)
        return [NSData data];                                                   // returns empty data object - means no error, but no data 
    
    //----- otherwise we had an error, return an error
    [self streamError: request errorCode: kBRFTPClientCantReadStream];
    InfoLog(@"%@", request.error.message);
    
    return nil;
}



//-----
//
//				write
//
// synopsis:	retval = [self write:request data:data];
//					BOOL retval       	-
//					BRRequest *request	-
//					NSData *data      	-
//
// description:	write is designed to
//
// errors:		none
//
// returns:		Variable of type BOOL
//

- (BOOL)write:(BRRequest *)request data:(NSData *)data
{
    bytesThisIteration = [writeStream write: [data bytes] maxLength: [data length]];
    bytesTotal += bytesThisIteration;
            
    if (bytesThisIteration > 0)
    {
        request.percentCompleted = bytesTotal / request.maximumSize;
        if ([request.delegate respondsToSelector:@selector(percentCompleted:)])
        {
            [request.delegate percentCompleted: request];
        }
        
        return TRUE;
    }
    
    if (bytesThisIteration == 0)
        return TRUE;
    
    [self streamError: request errorCode: kBRFTPClientCantWriteStream]; // perform callbacks and close out streams
    InfoLog(@"%@", request.error.message);

    return FALSE;
}



//-----
//
//				streamError
//
// synopsis:	[self streamError:request errorCode:errorCode];
//					BRRequest *request         	-
//					enum BRErrorCodes errorCode	-
//
// description:	streamError is designed to
//
// errors:		none
//
// returns:		none
//

- (void)streamError:(BRRequest *)request errorCode:(enum BRErrorCodes)errorCode
{
    request.error = [[BRRequestError alloc] init];
    request.error.errorCode = errorCode;
    [request.delegate requestFailedBR: request];
    [request.streamInfo close: request];
}



//-----
//
//				streamComplete
//
// synopsis:	[self streamComplete:request];
//					BRRequest *request	-
//
// description:	streamComplete is designed to
//
// errors:		none
//
// returns:		none
//

- (void)streamComplete:(BRRequest *)request
{
    [request.delegate requestCompletedBR: request];
    [request.streamInfo close: request];
}



//-----
//
//				close
//
// synopsis:	[self close:request];
//					BRRequest *request	-
//
// description:	close is designed to
//
// errors:		none
//
// returns:		none
//

- (void)close:(BRRequest *)request
{
    if (readStream)
    {
        [readStream close];
        [readStream removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        readStream = nil;
    }
    
    if (writeStream)
    {
        [writeStream close];
        [writeStream removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        writeStream = nil;
    }
    
    request.streamInfo = nil;
}

@end
