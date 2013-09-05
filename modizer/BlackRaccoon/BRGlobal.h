//----------
//
//				BRGlobal.h
//
// filename:	BRGlobal.h
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



//---------- enumerated data types



//---------- typedefs
typedef enum 
{
    kBRUploadRequest,
	kBRDownloadRequest,
    kBRDeleteRequest,
    kBRCreateDirectoryRequest,
	kBRListDirectoryRequest
} BRRequestTypes;


typedef enum 
{
    kBRDefaultBufferSize = 32768
} BRBufferSizes;


typedef enum BRTimeouts
{
    kBRDefaultTimeout = 30
} BRTimeouts;

typedef enum BRErrorCodes
{
    //client errors
    kBRFTPClientHostnameIsNil = 901,
    kBRFTPClientCantOpenStream = 902,
    kBRFTPClientCantWriteStream = 903,
    kBRFTPClientCantReadStream = 904,
    kBRFTPClientSentDataIsNil = 905,    
    kBRFTPClientFileAlreadyExists = 907,
    kBRFTPClientCantOverwriteDirectory = 908,
    kBRFTPClientStreamTimedOut = 909,
    kBRFTPClientCantDeleteFileOrDirectory = 910,
    kBRFTPClientMissingRequestDataAvailable = 911,
    
    // 400 FTP errors
    kBRFTPServerAbortedTransfer = 426,
    kBRFTPServerResourceBusy = 450,
    kBRFTPServerCantOpenDataConnection = 425,
    
    // 500 FTP errors
    kBRFTPServerUserNotLoggedIn = 530,
    kBRFTPServerFileNotAvailable = 550,
    kBRFTPServerStorageAllocationExceeded = 552,
    kBRFTPServerIllegalFileName = 553,
    kBRFTPServerUnknownError
    
} BRErrorCodes;



//---------- definitions
#ifdef DEBUG
#	define InfoLog(fmt, ...) NSLog((@"%s [Line %d] " fmt), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#	define InfoLog(...)
#endif



//---------- structs



//---------- external functions



//---------- external variables



//---------- global functions



//---------- local functions



//---------- global variables



//---------- local variables



//---------- protocols



//---------- classes

