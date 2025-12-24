//
//  RootViewControllerP2612WebParser.h
//  modizer1
//
//  Created by Yohann Magnien on 07/05/21.
//  Copyright __YoyoFR / Yohann Magnien__ 2021. All rights reserved.
//

#import "RootViewControllerXPWebParser.h"

@class DetailViewControllerIphone;
@class DownloadViewController;


@interface RootViewControllerAMPWebParser : RootViewControllerXPWebParser <UINavigationControllerDelegate,UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
    
    NSMutableArray *arr_url_handleList,*arr_url_realnameList,*arr_url_countryList,*arr_url_groupsList;
    int browse_subMode;
}

@property (nonatomic, strong) NSMutableArray *arr_url_handleList,*arr_url_realnameList,*arr_url_countryList,*arr_url_groupsList;
@property (nonatomic, assign) int browse_subMode;

@end
