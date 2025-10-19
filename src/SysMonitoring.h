//
//  SysMonitoring.h
//  modizer
//
//  Created by Yohann Magnien David on 19/10/2025.
//

#ifndef __SysMonitoring_h__
#define __SysMonitoring_h__

#import <Foundation/Foundation.h>

@interface SysMonitoring : NSObject

@property (nonatomic, strong) NSTimer *timer;
@property double cpuUsage;

- (void)startMonitoring;
- (void)stopMonitoring;
- (double)getCPUUsage;

@end

#endif
