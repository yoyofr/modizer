//----------
//
//				BRRequest+_UserData.h
//
// filename:	BRRequest+_UserData.h
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
#include "BRRequest.h"



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

@interface BRRequest (_UserData)

@property BOOL userCancelRequest;
@property NSString *uuid;

- (void) cancelRequestWithFlag;

@end
