//
//  VoiceCommandsViewController.h
//  modizer
//
//  Created for Siri voice commands display
//

#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"

@class DetailViewControllerIphone;

@interface VoiceCommandsViewController : UIViewController <UITableViewDelegate, UITableViewDataSource> {
    DetailViewControllerIphone *detailViewController;
    UITableView *tableView;

    NSArray *commandCategories;
    NSArray *commandsByCategory;

    bool darkMode;

    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;

    WaitingView *waitingView, *waitingViewPlayer;
    NSTimer *repeatingTimer;
}

@property (nonatomic, retain) DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) NSArray *commandCategories;
@property (nonatomic, retain) NSArray *commandsByCategory;

-(IBAction) goPlayer;
-(void) updateMiniPlayer;

@end
