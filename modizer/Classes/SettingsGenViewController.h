//
//  SettingsGenViewController.h
//  modizer
//
//  Created by Yohann Magnien on 10/08/13.
//
//

#import <UIKit/UIKit.h>
#import "ModizerConstants.h"
#import "DetailViewControllerIphone.h"




@interface SettingsGenViewController : UIViewController {
    IBOutlet UITableView *tableView;
    IBOutlet UISearchBar *sBar;
@public
    IBOutlet DetailViewControllerIphone *detailViewController;
}

@property (nonatomic,retain) IBOutlet UITableView *tableView;
@property (nonatomic,retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic,retain) IBOutlet UISearchBar *sBar;

@end
