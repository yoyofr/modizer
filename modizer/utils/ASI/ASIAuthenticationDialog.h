//
//  ASIAuthenticationDialog.h
//  Part of ASIHTTPRequest -> http://allseeing-i.com/ASIHTTPRequest
//
//  Created by Ben Copsey on 21/08/2009.
//  Copyright 2009 All-Seeing Interactive. All rights reserved.
//

//  Modifyed for ARC by Jayesh on 19/06/14.
//  Copyright (c) 2012 jayeshkavathiya@gmail.com.

#import <Foundation/Foundation.h>
@class ASIHTTPRequest;

typedef enum _ASIAuthenticationType {
	ASIStandardAuthenticationType = 0,
    ASIProxyAuthenticationType = 1
} ASIAuthenticationType;
	
@interface ASIAuthenticationDialog : NSObject <UIActionSheetDelegate, UITableViewDelegate, UITableViewDataSource> {
	ASIHTTPRequest *request;
	UIActionSheet *loginDialog;
	ASIAuthenticationType type;
}
+ (void)presentAuthenticationDialogForRequest:(ASIHTTPRequest *)request;
+ (void)presentProxyAuthenticationDialogForRequest:(ASIHTTPRequest *)request;

@property (retain) ASIHTTPRequest *request;
@property (retain) UIActionSheet *loginDialog;
@property (assign) ASIAuthenticationType type;
@end
