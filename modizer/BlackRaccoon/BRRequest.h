//----------
//
//				BRRequest.h
//
// filename:	BRRequest.h
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

/// It also defines the protocol. The three required delegate methods for
/// BlackRaccoon are:
///
/// * **requestCompletedBR:request** - this indicates that the request was completed
/// successfully. Note, depending on the FTP server you may get a successful
/// completion even though what you expected to happen failed. This is rare,
/// fortunately.
///
/// * **requestFailedBR:request** - this indicates that the request failed. You can
/// examine the **error** object for further details of why it failed.
///
/// * **shouldOverwriteFileWithRequest:request** - the users code should return a
/// boolean value indicating whether or not is is okay to overwrite the data.
/// TYPICALLY the user will return no.
///
/// There are also four **optional** delegate methods are:
///
/// * **percentCompleted:request** - this is called on every packet sent or
/// recieved so the user can display, in real time, the percentage of the of the
/// file uploaded/downloaded. If this is not required for the application, it is
/// not required that it is implemented.
///
/// * **requestDataAvailable:request** -
///
/// * **requestDataSendSize:request** -
///
/// * **requestDataToSend:request** -
///



//---------- pragmas



//---------- include files
#import "BRGlobal.h"
#import "BRRequestError.h"
#import "BRStreamInfo.h"


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
@class BRRequest;
@class BRRequestDownload;
@class BRRequestUpload;
@protocol BRRequestDelegate  <NSObject>

@required
/// requestCompletedBR
/// Indicates when a request has completed without errors.
/// \param request The request object
- (void)requestCompletedBR:(BRRequest *)request;

/// requestFailedBR
/// \param request The request object
- (void)requestFailedBR:(BRRequest *)request;

/// shouldOverwriteFileWithRequest
/// \param request The request object;
- (BOOL)shouldOverwriteFileWithRequest:(BRRequest *)request;

@optional
- (void)percentCompleted:(BRRequest *) request;
- (void)requestDataAvailable:(BRRequestDownload *)request;
- (long)requestDataSendSize:(BRRequestUpload *)request;
- (NSData *)requestDataToSend:(BRRequestUpload *)request;
@end

//---------- classes

/// BRRequest is the main structure used throughout BlackRaccoon. It contains
/// important items such as the username, password, hostname, percent complete,
/// etc.

@interface BRRequest : NSObject <NSStreamDelegate>
{
@protected
    NSString * path;
    NSString * hostname;
    
    /// Error code and string.
    BRRequestError *error;
}

@property BOOL passiveMode;

@property NSMutableDictionary *userDictionary;                                  // contains user values

/// String used to log into the server.
@property NSString *username;

/// Password used to log into the server.
@property NSString *password;

/// Ftp host name.
@property NSString *hostname;

/// URL to the ftp host.
@property (readonly) NSURL *fullURL;
@property NSString *path;
@property (strong) BRRequestError *error;
@property float maximumSize;
@property float percentCompleted;
@property long timeout;

@property BRRequest *nextRequest;
@property BRRequest *prevRequest;
@property (weak) id <BRRequestDelegate> delegate;
@property  BRStreamInfo *streamInfo;
@property BOOL didOpenStream;                                                   // whether the stream opened or not
@property (readonly) long bytesSent;                                            // will have bytes from the last FTP call
@property (readonly) long totalBytesSent;                                       // will have bytes total sent
@property BOOL cancelDoesNotCallDelegate;                                       // cancel closes stream without calling delegate

- (NSURL *)fullURLWithEscape;

- (instancetype)initWithDelegate:(id)delegate;

- (void)start;
- (void)cancelRequest;

@end
