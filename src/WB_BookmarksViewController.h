//
//  WB_BookmarksViewController.h
//  modizer
//
//  Created by Yohann Magnien on 07/08/13.
//
//

#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"
#import "WebBrowser.h"

#define MAX_CUSTOM_URL 256
#define MAX_BUILTIN_URL 64

@interface WB_BookmarksViewController : UIViewController {
    IBOutlet UITableView *tableView;
    IBOutlet UIToolbar *toolBar;
    int builtin_url_count;
    int list_builtin;
    NSString *builtin_URL[MAX_BUILTIN_URL];
	NSString *builtin_URL_name[MAX_BUILTIN_URL];
    
    int custom_url_count;
    NSString *custom_URL[MAX_CUSTOM_URL];
	NSString *custom_URL_name[MAX_CUSTOM_URL];

@public
    DetailViewControllerIphone *detailViewController;
    WebBrowser *webBrowser;
}
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UIToolbar *toolBar;
@property (nonatomic, retain) WebBrowser *webBrowser;

-(IBAction) closeBookmarks;
-(IBAction) editBookmarks;

@end
