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
@interface MoreViewController : UITableViewController <UINavigationControllerDelegate>{
    IBOutlet DownloadViewController *downloadViewController;
    IBOutlet DetailViewControllerIphone *detailViewController;
    IBOutlet UITableView *tableView;
    IBOutlet AboutViewController *aboutVC;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    WaitingView *waitingView;
    
    bool darkMode;
    bool forceReloadCells;
    
    IBOutlet RootViewControllerLocalBrowser *rootVC;
}
@property (nonatomic, retain) IBOutlet DownloadViewController *downloadViewController;
@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet AboutViewController *aboutVC;
@property (nonatomic, retain) IBOutlet RootViewControllerLocalBrowser *rootVC;

-(IBAction) goPlayer;
-(void) refreshViewAfterDownload;
-(void) updateMiniPlayer;

@end
