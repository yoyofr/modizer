//
//  main.mm
//  modizer
//
//  Created by yoyofr on 7/28/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate_Phone.h"
#import "SettingsGenViewController.h"

os_log_t mdzLog;

int main(int argc, char *argv[]) {
    
    @autoreleasepool {
        //***********************
        //init stuff
        //***********************
        //settings
        
        mdzLog = os_log_create("com.yoyofr.modizer", "global");

                
        START_PROFILE
        [SettingsGenViewController loadSettings];
        [SettingsGenViewController restoreSettings];
        //Check if FTP should be started
        [SettingsGenViewController FTPcheckStatus];
        END_PROFILE
        
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate_Phone class]));
    }
    //NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    //int retVal = UIApplicationMain(argc, argv, nil, nil);
    //[pool release];
    //return retVal;
}
