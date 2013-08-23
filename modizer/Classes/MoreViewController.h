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

@interface MoreViewController : UITableViewController {
    IBOutlet DetailViewControllerIphone *detailViewController;
    IBOutlet UITableView *tableView;
    IBOutlet AboutViewController *aboutVC;
    IBOutlet RootViewControllerLocalBrowser *rootVC;
}
@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet AboutViewController *aboutVC;
@property (nonatomic, retain) IBOutlet RootViewControllerLocalBrowser *rootVC;

-(IBAction) goPlayer;

@end
