//----------
//
//				BRRequest+_UserData.m
//
// filename:	BRRequest+_UserData.m
//
// author:		Lloyd Sargent
//
// created:		Jul 01, 2013
//
// description:
//
// notes:		none
//
// revisions:
//
// Copyright (c) 2013 Canna Software. All rights reserved.
//



//---------- pragmas



//---------- include files
#import "BRRequest+_UserData.h"



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

@implementation BRRequest (_UserData)



//-----
//
//				setUserCancelRequest
//
// synopsis:	[self setUserCancelRequest:value];
//					BOOL value	-
//
// description:	setUserCancelRequest is designed to
//
// errors:		none
//
// returns:		none
//

- (void) setUserCancelRequest: (BOOL) value
{
    if (value)
        [self.userDictionary setObject: @"cancel" forKey: @"cancel"];
    else
        [self.userDictionary removeObjectForKey: @"cancel"];
}



//-----
//
//				userCancelRequest
//
// synopsis:	retval = [self userCancelRequest];
//					BOOL retval	-
//
// description:	userCancelRequest is designed to
//
// errors:		none
//
// returns:		Variable of type BOOL
//

- (BOOL) userCancelRequest
{
    return [self.userDictionary objectForKey: @"cancel"] != nil;
}



//-----
//
//				cancelRequestWithFlag
//
// synopsis:	[self cancelRequestWithFlag];
//
// description:	cancelRequestWithFlag is designed to
//
// errors:		none
//
// returns:		none
//

- (void) cancelRequestWithFlag
{
    self.userCancelRequest = TRUE;
    [self cancelRequest];
}



//-----
//
//				setUuid
//
// synopsis:	[self setUuid:value];
//					NSString *value	-
//
// description:	setUuid is designed to
//
// errors:		none
//
// returns:		none
//

- (void) setUuid: (NSString *) value
{
    [self.userDictionary setObject: value forKey: @"uuid"];
}



//-----
//
//				uuid
//
// synopsis:	retval = [self uuid];
//					NSString *retval	-
//
// description:	uuid is designed to
//
// errors:		none
//
// returns:		Variable of type NSString *
//

- (NSString *) uuid
{
    return [self.userDictionary objectForKey: @"uuid"];
}

@end
