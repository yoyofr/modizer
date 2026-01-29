//
//  RootViewControllerJoshWWebParser.h
//  modizer
//
//  Created by Yohann Magnien on 07/05/21.
//  Copyright __YoyoFR / Yohann Magnien__ 2021. All rights reserved.
//

#import "RootViewControllerXPWebParser.h"
#import "CoverScrapper.h"

@class DetailViewControllerIphone;
@class DownloadViewController;

@interface RootViewControllerJoshWWebParser : RootViewControllerXPWebParser <UINavigationControllerDelegate,UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
@public
    NSString *mWebBaseDir;
    NSData  *urlData[27+4];
    int data_cnt;
}

@property (nonatomic, retain) NSString *mWebBaseDir;
@property (nonatomic, strong) CoverScrapper *scrapper;

@end
