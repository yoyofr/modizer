//----------
//
//				BRRequestDelete.m
//
// filename:	BRRequestDelete.m
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
#import "BRRequestDelete.h"



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

@implementation BRRequestDelete


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
    NSString * lastCharacter = [path substringFromIndex:[path length] - 1];
    isDirectory = ([lastCharacter isEqualToString:@"/"]);
    
    if (!isDirectory) 
        return [super path];
    
    NSString * directoryPath = [super path];
    if (![directoryPath isEqualToString:@""]) 
    {
        directoryPath = [directoryPath stringByAppendingString:@"/"];
    }
    
    return directoryPath;
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
    SInt32 errorcode;
    
    if (self.hostname==nil) 
    {
        InfoLog(@"The host name is nil!");
        [self.streamInfo streamError: self errorCode: kBRFTPClientHostnameIsNil];
        return;
    }
    
    if (CFURLDestroyResource(( __bridge CFURLRef) self.fullURLWithEscape, &errorcode))
    {
        //----- successful
        [self.streamInfo streamComplete: self];
    }
    
    else 
    {
        //----- unsuccessful        
        [self.streamInfo streamError: self errorCode: kBRFTPClientCantDeleteFileOrDirectory];
    }
}

@end
