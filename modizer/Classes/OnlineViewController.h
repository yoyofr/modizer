//
//  OnlineViewController.h
//  modizer
//
//  Created by Yohann Magnien on 29/07/13.
//
//

#import <UIKit/UIKit.h>

#import "ModizerConstants.h"
#import "DownloadViewController.h"
#import "RootViewControllerMODLAND.h"
#import "RootViewControllerHVSC.h"
#import "WebBrowser.h"

@interface OnlineViewController : UITableViewController {

    IBOutlet DownloadViewController *downloadViewController;
    IBOutlet WebBrowser *webBrowser;
    IBOutlet DetailViewControllerIphone *detailViewController;
    IBOutlet UIBarButtonItem *playerButton;
    IBOutlet UITableView *tableView;
    
    int mNbMODLANDFileEntries,mNbHVSCFileEntries;
	
    
    UIViewController *collectionViewController;
}

@property (nonatomic, retain) IBOutlet DownloadViewController *downloadViewController;
@property (nonatomic, retain) IBOutlet WebBrowser *webBrowser;
@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UIBarButtonItem *playerButton;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) UIViewController *collectionViewController;
@property (nonatomic) int mNbMODLANDFileEntries,mNbHVSCFileEntries;


-(IBAction) goPlayer;

@end
