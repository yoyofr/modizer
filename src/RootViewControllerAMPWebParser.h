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
    
    NSMutableArray *arr_url_handleList,*arr_url_realnameList,*arr_url_countryList,*arr_url_groupsList,*arr_url_groupsLogoList;
    NSMutableArray *arr_url_fileList,*arr_url_composerList,*arr_url_formatList,*arr_url_sizeList;
    int browse_subMode;
}

@property (nonatomic, strong) NSMutableArray *arr_url_handleList,*arr_url_realnameList,*arr_url_countryList,*arr_url_groupsList,*arr_url_groupsLogoList;
@property (nonatomic, strong) NSMutableArray *arr_url_fileList,*arr_url_composerList,*arr_url_formatList,*arr_url_sizeList;
@property (nonatomic, assign) int browse_subMode;

@end
