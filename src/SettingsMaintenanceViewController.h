//
//  SettingsMaintenanceViewController.h
//  modizer
//
//  Created by Yohann Magnien on 10/08/13.
//
//

#import <UIKit/UIKit.h>
#import "ModizerConstants.h"
#import "DetailViewControllerIphone.h"
#import "SettingsGenViewController.h"
#import "RootViewControllerLocalBrowser.h"

@interface SettingsMaintenanceViewController : UIViewController <UIAlertViewDelegate> {
    IBOutlet UITableView *tableView;
    UIView *waitingView;
    
    bool darkMode;
    bool forceReloadCells;

@public
    DetailViewControllerIphone *detailViewController;
    RootViewControllerLocalBrowser *rootVC;
}

@property (nonatomic,retain) IBOutlet UITableView *tableView;
@property (nonatomic,retain) DetailViewControllerIphone *detailViewController;
@property (nonatomic,retain) RootViewControllerLocalBrowser *rootVC;

@end
