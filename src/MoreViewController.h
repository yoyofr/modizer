//
//  MoreViewController.h
//  modizer
//
//  Created by Yohann Magnien on 08/08/13.
//
//

#import <UIKit/UIKit.h>

#import "ModizerConstants.h"
#import "DownloadViewController.h"
#import "AboutViewController.h"
#import "SettingsMaintenanceViewController.h"
#import "RootViewControllerLocalBrowser.h"
#import "DownloadViewController.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"


@class DownloadViewController;
@interface MoreViewController : UIViewController <UINavigationControllerDelegate>{
    DownloadViewController *downloadViewController;
    DetailViewControllerIphone *detailViewController;
    RootViewControllerLocalBrowser *rootVC;
    AboutViewController *aboutVC;
    
    IBOutlet UITableView *tableView;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    WaitingView *waitingView,*waitingViewPlayer;
    NSTimer *repeatingTimer;
    
    bool darkMode;
    bool forceReloadCells;
    
    
}
@property (nonatomic, retain) DownloadViewController *downloadViewController;
@property (nonatomic, retain) DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) AboutViewController *aboutVC;
@property (nonatomic, retain) RootViewControllerLocalBrowser *rootVC;

@property (nonatomic, retain) IBOutlet UITableView *tableView;

-(IBAction) goPlayer;
-(void) refreshViewAfterDownload;
-(void) updateMiniPlayer;

@end
