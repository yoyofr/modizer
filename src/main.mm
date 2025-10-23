//
//  main.m
//  modizer
//
//  Created by yoyofr on 7/28/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//
#import <OSLog/OSLog.h>
#import <UIKit/UIKit.h>
#import "AppDelegate_Phone.h"
#import "SettingsGenViewController.h"


int main(int argc, char *argv[]) {
    
    @autoreleasepool {
        //***********************
        //init stuff
        //***********************
        //settings
                
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
