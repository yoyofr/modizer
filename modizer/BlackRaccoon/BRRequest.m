//----------
//
//				BRRequest.m
//
// filename:	BRRequest.m
//
// author:		Created by Valentin Radu on 8/23/11.
//              Copyright 2011 Valentin Radu. All rights reserved.
//
//              Modified and/or redesigned by Lloyd Sargent to be ARC compliant.
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

@implementation BRRequest

@synthesize passiveMode;
@synthesize password;
@synthesize username;
@synthesize error;
@synthesize maximumSize;
@synthesize percentCompleted;

@synthesize nextRequest;
@synthesize prevRequest;
@synthesize delegate;
@synthesize streamInfo;
@synthesize didOpenStream;


//-----
//
//				initWithDelegate
//
// synopsis:	retval = [BRRequest delegate];
//					BRRequestCreateDirectory *retval	-
//					id<BRRequestDelegate> delegate      -
//
// description:	initWithDelegate is designed to
//
// errors:		none
//
// returns:		Instance of type BRRequest or subclass
//

- (id)initWithDelegate:(id<BRRequestDelegate>)aDelegate
{
    self = [super init];
    if (self)
    {
		self.passiveMode = YES;
        self.userDictionary = [NSMutableDictionary dictionaryWithCapacity: 1];
        self.password = nil;
        self.username = nil;
        self.hostname = nil;
        self.path = @"";
        
        streamInfo = [[BRStreamInfo alloc] init];
        self.streamInfo.readStream = nil;
        self.streamInfo.writeStream = nil;
        self.streamInfo.bytesThisIteration = 0;
        self.streamInfo.bytesTotal = 0;
        self.streamInfo.timeout = 30;
        
        self.delegate = aDelegate;
    }
    return self;
}


//-----
//
//				fullURL
//
// synopsis:	retval = [self fullURL];
//					NSURL*retval	-
//
// description:	fullURL is designed to
//
// errors:		none
//
// returns:		Variable of type NSURL*
//

- (NSURL *)fullURL
{
    NSString * fullURLString = [NSString stringWithFormat: @"ftp://%@%@", self.hostname, self.path];
    
    return [NSURL URLWithString: fullURLString];
}

//-----
//
//				fullURLWithEscape
//
// synopsis:	retval = [self fullURLWithEscape];
//					NSURL *retval	-
//
// description:	fullURLWithEscape is designed to
//
// errors:		none
//
// returns:		Variable of type NSURL *
//

- (NSURL *)fullURLWithEscape
{
    NSString *escapedUsername = [self encodeString: username];
    NSString *escapedPassword = [self encodeString: password];
    NSString *cred;
    
    if (escapedUsername != nil)
    {
        if (escapedPassword != nil)
        {
            cred = [NSString stringWithFormat:@"%@:%@@", escapedUsername, escapedPassword];
        }else
        {
            cred = [NSString stringWithFormat:@"%@@", escapedUsername];
        }
    }
    else
    {
        cred = @"";
    }
    cred = [cred stringByStandardizingPath];
    
    NSString * fullURLString = [NSString stringWithFormat:@"ftp://%@%@%@", cred, self.hostname, self.path];
    
    return [NSURL URLWithString: fullURLString];
}

//-----
//
//				path
//
// synopsis:	retval = [self path];
//					NSString *retval	-
//
// description:	path is designed to
//
// errors:		none
//
// returns:		Variable of type NSString *
//

- (NSString *)path
{
    //  we remove all the extra slashes from the directory path, including the last one (if there is one)
    //  we also escape it
    NSString * escapedPath = [path stringByStandardizingPath];
    
    
    //  we need the path to be absolute, if it's not, we *make* it
    if (![escapedPath isAbsolutePath])
    {
        escapedPath = [@"/" stringByAppendingString:escapedPath];
    }
    
    //----- now make sure that we have escaped all special characters
    escapedPath = [self encodeString: escapedPath];
    
    return escapedPath;
}



//-----
//
//				setPath
//
// synopsis:	[self setPath:directoryPathLocal];
//					NSString *directoryPathLocal	-
//
// description:	setPath is designed to
//
// errors:		none
//
// returns:		none
//

- (void)setPath:(NSString *)directoryPathLocal
{
    path = directoryPathLocal;
}



//-----
//
//				hostname
//
// synopsis:	retval = [self hostname];
//					NSString *retval	-
//
// description:	hostname is designed to
//
// errors:		none
//
// returns:		Variable of type NSString *
//

- (NSString *)hostname
{
    return [hostname stringByStandardizingPath];
}



//-----
//
//				setHostname
//
// synopsis:	[self setHostname:hostnamelocal];
//					NSString *hostnamelocal	-
//
// description:	setHostname is designed to
//
// errors:		none
//
// returns:		none
//

- (void)setHostname:(NSString *)hostnamelocal
{
    hostname = hostnamelocal;
}



//-----
//
//				encodeString
//
// synopsis:	retval = [self encodeString:string];
//					NSString *retval	-
//					NSString *string	-
//
// description:	encodeString is designed to
//
// errors:		none
//
// returns:		Variable of type NSString *
//

- (NSString *)encodeString:(NSString *)string;
{
    NSString *urlEncoded = (__bridge_transfer NSString *)CFURLCreateStringByAddingPercentEscapes(
                                                                                                 NULL,
                                                                                                 (__bridge CFStringRef) string,
                                                                                                 NULL,
                                                                                                 (CFStringRef)@"!*'\"();:@&=+$,?%#[]% ",
                                                                                                 kCFStringEncodingUTF8);
    return urlEncoded;
}  



//-----
//
//				start
//
// synopsis:	[self start];
//
// description:	start is designed to
//
// errors:		none
//
// returns:		none
//

- (void)start
{
    
}



//-----
//
//				bytesSent
//
// synopsis:	retval = [self bytesSent];
//					long retval	-
//
// description:	bytesSent is designed to
//
// errors:		none
//
// returns:		Variable of type long
//

- (long)bytesSent
{
    return self.streamInfo.bytesThisIteration;
}



//-----
//
//				totalBytesSent
//
// synopsis:	retval = [self totalBytesSent];
//					long retval	-
//
// description:	totalBytesSent is designed to
//
// errors:		none
//
// returns:		Variable of type long
//

- (long)totalBytesSent
{
    return self.streamInfo.bytesTotal;
}



//-----
//
//				timeout
//
// synopsis:	retval = [self timeout];
//					long retval	-
//
// description:	timeout is designed to
//
// errors:		none
//
// returns:		Variable of type long
//

- (long)timeout
{
    return self.streamInfo.timeout;
}



//-----
//
//				setTimeout
//
// synopsis:	[self setTimeout:timeout];
//					long timeout	-
//
// description:	setTimeout is designed to
//
// errors:		none
//
// returns:		none
//

- (void)setTimeout:(long)timeout
{
    self.streamInfo.timeout = timeout;
}



//-----
//
//				cancelRequest
//
// synopsis:	[self cancelRequest];
//
// description:	cancelRequest is designed to
//
// errors:		none
//
// returns:		none
//

- (void)cancelRequest
{
    self.streamInfo.cancelRequestFlag = TRUE;
}



//-----
//
//				setCancelDoesNotCallDelegate
//
// synopsis:	[self setCancelDoesNotCallDelegate:cancelDoesNotCallDelegate];
//					BOOL cancelDoesNotCallDelegate	-
//
// description:	setCancelDoesNotCallDelegate is designed to
//
// errors:		none
//
// returns:		none
//

- (void)setCancelDoesNotCallDelegate:(BOOL)cancelDoesNotCallDelegate
{
    self.streamInfo.cancelDoesNotCallDelegate = cancelDoesNotCallDelegate;
}



//-----
//
//				cancelDoesNotCallDelegate
//
// synopsis:	retval = [self cancelDoesNotCallDelegate];
//					BOOL retval	-
//
// description:	cancelDoesNotCallDelegate is designed to
//
// errors:		none
//
// returns:		Variable of type BOOL
//

- (BOOL)cancelDoesNotCallDelegate
{
    return self.streamInfo.cancelDoesNotCallDelegate;
}


@end
