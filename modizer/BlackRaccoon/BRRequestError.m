//----------
//
//				BRRequestError.m
//
// filename:	BRRequestError.m
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
#import "BRRequestError.h"



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

@implementation BRRequestError

@synthesize errorCode;




//-----
//
//				errorCodeWithError
//
// synopsis:	retval = [BRRequestError errorCodeWithError:error];
//					BRErrorCodes retval	-
//					NSError *error     	-
//
// description:	errorCodeWithError is designed to
//
// errors:		none
//
// returns:		Variable of type BRErrorCodes
//

+(BRErrorCodes) errorCodeWithError: (NSError *) error
{
    //----- As suggested by RMaddy
    NSDictionary *userInfo = error.userInfo;
    NSNumber *code = [userInfo objectForKey:(id)kCFFTPStatusCodeKey];
    
    if (code)
    {
        return [code intValue];
    }
    
    return 0;
}



//-----
//
//				message
//
// synopsis:	retval = [self message];
//					NSString *retval	-
//
// description:	message is designed to
//
// errors:		none
//
// returns:		Variable of type NSString *
//

- (NSString *)message
{
    NSString * errorMessage;
    switch (self.errorCode) 
    {
        //----- Client errors
        case kBRFTPClientSentDataIsNil:
            errorMessage = @"Data is nil.";
            break;
            
        case kBRFTPClientCantOpenStream:
            errorMessage = @"Unable to open stream.";
            break;
            
        case kBRFTPClientCantWriteStream:
            errorMessage = @"Unable to write to open stream.";
            break;
            
        case kBRFTPClientCantReadStream:
            errorMessage = @"Unable to read from open stream.";
            break;
            
        case kBRFTPClientHostnameIsNil:
            errorMessage = @"Hostname is nil.";
            break;
            
        case kBRFTPClientFileAlreadyExists:
            errorMessage = @"File already exists!";
            break;
            
        case kBRFTPClientCantOverwriteDirectory:
            errorMessage = @"Can't overwrite directory!";
            break;
            
        case kBRFTPClientStreamTimedOut:
            errorMessage = @"Stream timed out with no response from server.";
            break;
            
        case kBRFTPClientCantDeleteFileOrDirectory:
            errorMessage = @"Can't delete file or directory.";
            break;
            
        case kBRFTPClientMissingRequestDataAvailable:
            errorMessage = @"Delegate missing requestDataAvailable:";
            break;
            
        //----- Server errors    
        case kBRFTPServerAbortedTransfer:
            errorMessage = @"Server aborted transfer.";
            break;
            
        case kBRFTPServerResourceBusy:
            errorMessage = @"Resource is busy.";
            break;
            
        case kBRFTPServerCantOpenDataConnection:
            errorMessage = @"Server can't open data connection.";
            break;
            
        case kBRFTPServerUserNotLoggedIn:
            errorMessage = @"Not logged in.";
            break;
            
        case kBRFTPServerStorageAllocationExceeded:
            errorMessage = @"Server allocation exceeded!";
            break;
            
        case kBRFTPServerIllegalFileName:
            errorMessage = @"Illegal file name.";
            break;
            
        case kBRFTPServerFileNotAvailable:
            errorMessage = @"File or directory not available or directory already exists.";
            break;
            
        case kBRFTPServerUnknownError:
            errorMessage = @"Unknown FTP error!";
            break;
            
        default:
            errorMessage = @"Unknown error!";
            break;
    }
    
    return errorMessage;
}

@end
