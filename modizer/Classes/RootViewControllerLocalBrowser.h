//
//  RootViewControllerLocalBrowser.h
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//


#import <UIKit/UIKit.h>
#import <sqlite3.h>
#import "DBHelper.h"
#import "RootViewControllerStruct.h"
#import "fex.h"
#import "CMPopTipView.h"

//#import "SWTableViewCell.h"
#import "SESlideTableViewCell.h"

@class DetailViewControllerIphone;

@interface RootViewControllerLocalBrowser : UIViewController <UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate,SESlideTableViewCellDelegate> {
	NSString *ratingImg[6];
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
    NSFileManager *mFileMngr;
	
    CMPopTipView *popTipView;
    int popTipViewRow,popTipViewSection;
	
	UIView *waitingView;
    
    UIAlertView *alertRename;
    int renameFile,renameSec,renameIdx;
    int createFolder;

	
	IBOutlet UISearchBar *sBar;
    IBOutlet UITableView *tableView;
    
    NSMutableArray *list;
    NSMutableArray *keys;
	NSMutableArray *indexTitles,*indexTitlesSpace;
		
	int mShowSubdir,shouldFillKeys;
    
    int mAccessoryButton;

	t_local_browse_entry *local_entries_data;
	int local_entries_count[27];
    t_local_browse_entry *local_entries[27];
	int local_nb_entries;
	
	t_local_browse_entry *search_local_entries_data;
	int search_local_entries_count[27];
    t_local_browse_entry *search_local_entries[27];
	int search_local_nb_entries;
	
	int search_local;
		
	int mSlowDevice;
	
	NSString *currentPath;
	int mSearch;
	NSString *mSearchText;
	
	UIViewController *childController;
	

	int mClickedPrimAction;
	int mCurrentWinAskedDownload;
@public    
    int browse_depth;
    IBOutlet DetailViewControllerIphone *detailViewController;	
}

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UIViewController *childController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;

@property (nonatomic, retain) NSArray *list;
@property (nonatomic, retain) NSArray *keys;
@property (nonatomic, retain) NSString *currentPath,*mSearchText;

@property (nonatomic, retain) NSFileManager *mFileMngr;
@property (nonatomic, retain) CMPopTipView *popTipView;

@property (nonatomic, retain) UIAlertView *alertRename;

-(IBAction)goPlayer;
-(void) refreshViewAfterDownload;
-(void)listLocalFiles;
-(void)createEditableCopyOfDatabaseIfNeeded:(bool)forceInit quiet:(int)quiet;
-(void)createSamplesFromPackage:(BOOL)forceCreate;

@end
