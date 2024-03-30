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
#import "MiniPlayerVC.h"
#import "WaitingView.h"

@interface SettingsMaintenanceViewController : UIViewController <UINavigationControllerDelegate,UIAlertViewDelegate> {
    IBOutlet UITableView *tableView;
    WaitingView *waitingView,*waitingViewPlayer;
    NSTimer *repeatingTimer;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    bool darkMode;
    bool forceReloadCells;

@public
    DetailViewControllerIphone *detailViewController;
    RootViewControllerLocalBrowser *rootVC;
}

@property (nonatomic,retain) IBOutlet UITableView *tableView;
@property (nonatomic,retain) DetailViewControllerIphone *detailViewController;
@property (nonatomic,retain) RootViewControllerLocalBrowser *rootVC;
@property (nonatomic, retain) WaitingView *waitingView,*waitingViewPlayer;

-(IBAction) goPlayer;
-(void) updateMiniPlayer;

@end
