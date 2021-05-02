//
//  CarPlayAndRemoteManagement.h
//  modizer
//
//  Created by Yohann Magnien on 23/04/2021.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <MediaPlayer/MediaPlayer.h>

#import "ModizerConstants.h"
#import "RootViewControllerLocalBrowser.h"


NS_ASSUME_NONNULL_BEGIN

@class DetailViewControllerIphone;

@interface CarPlayAndRemoteManagement : NSObject <MPPlayableContentDataSource,MPPlayableContentDelegate> {
    DetailViewControllerIphone *detailViewController;
    RootViewControllerLocalBrowser *rootVCLocalB;
    
    NSMutableArray *plArray;
    NSTimer *repeatingTimer;
}

@property (nonatomic, retain) DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) RootViewControllerLocalBrowser *rootVCLocalB;

-(bool) initCarPlayAndRemote;

@end

NS_ASSUME_NONNULL_END
