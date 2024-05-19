//
//  OnlineViewController.h
//  modizer
//
//  Created by Yohann Magnien on 29/07/13.
//
//

#import <UIKit/UIKit.h>

#import "ModizerConstants.h"
#import "WebBrowser.h"
#import "RootViewControllerMODLAND.h"
#import "RootViewControllerHVSC.h"
#import "RootViewControllerASMA.h"
#import "RootViewControllerJoshWWebParser.h"
#import "RootViewControllerVGMRWebParser.h"
#import "RootViewControllerP2612WebParser.h"
#import "RootViewControllerSNESMWebParser.h"
#import "RootViewControllerSMSPWebParser.h"
#import "RootViewControllerZXArtWebParser.h"
#import "DownloadViewController.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"

@interface OnlineViewController : UITableViewController <UINavigationControllerDelegate>{
    
    IBOutlet DownloadViewController *downloadViewController;
    IBOutlet WebBrowser *webBrowser;
    IBOutlet DetailViewControllerIphone *detailViewController;
    IBOutlet UITableView *tableView;
    
    WaitingView *waitingView,*waitingViewPlayer;
    NSTimer *repeatingTimer;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    int mNbMODLANDFileEntries,mNbHVSCFileEntries,mNbASMAFileEntries;
    
    bool darkMode;
    bool forceReloadCells;
    
    UIViewController *collectionViewController;
}

@property (nonatomic, retain) IBOutlet DownloadViewController *downloadViewController;
@property (nonatomic, retain) IBOutlet WebBrowser *webBrowser;
@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) UIViewController *collectionViewController;
@property (nonatomic) int mNbMODLANDFileEntries,mNbHVSCFileEntries,mNbASMAFileEntries;
@property (nonatomic, retain) WaitingView *waitingView,*waitingViewPlayer;


-(IBAction) goPlayer;
-(void) refreshViewAfterDownload;
-(void) updateMiniPlayer;

@end
