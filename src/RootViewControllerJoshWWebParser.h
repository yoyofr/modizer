//
//  RootViewControllerJoshWWebParser.h
//  modizer1
//
//  Created by Yohann Magnien on 07/05/21.
//  Copyright __YoyoFR / Yohann Magnien__ 2021. All rights reserved.
//

#import "RootViewControllerXPWebParser.h"

@class DetailViewControllerIphone;
@class DownloadViewController;


@interface RootViewControllerJoshWWebParser : RootViewControllerXPWebParser <UINavigationControllerDelegate,UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
@public
    NSString *mWebBaseDir;
    
}

@property (nonatomic, retain) NSString *mWebBaseDir;


@end
