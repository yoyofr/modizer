//
//  WB_BookmarksViewController.h
//  modizer
//
//  Created by Yohann Magnien on 07/08/13.
//
//

#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"
#import "WebBrowser.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"
#import "SESlideTableViewCell.h"

#define MAX_CUSTOM_URL 5

@interface WB_BookmarksViewController : UIViewController <UINavigationControllerDelegate,SESlideTableViewCellDelegate>{
    IBOutlet UITableView *tableView;
    IBOutlet UIToolbar *toolBar;
    
    WaitingView *waitingView,*waitingViewPlayer;
    NSTimer *repeatingTimer;

    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    bool darkMode;
    bool forceReloadCells;
    
    int custom_url_count;
    NSString *custom_URL[MAX_CUSTOM_URL];
	NSString *custom_URL_name[MAX_CUSTOM_URL];

@public
    DetailViewControllerIphone *detailViewController;
    WebBrowser *webBrowser;
}
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UIToolbar *toolBar;
@property (nonatomic, retain) WebBrowser *webBrowser;
@property (nonatomic, retain) WaitingView *waitingView,*waitingViewPlayer;

-(IBAction) closeBookmarks;
-(IBAction) editBookmarks;

@end
