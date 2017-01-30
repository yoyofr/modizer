//
//  WebBrowser.h
//  modizer4
//
//  Created by yoyofr on 7/4/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "DownloadViewController.h"
#import "DetailViewControllerIphone.h"

#define MAX_CUSTOM_URL 256

@class DownloadViewController;
@class DetailViewControllerIphone;

@interface WebBrowser : UIViewController <UIWebViewDelegate,UITextFieldDelegate> {
	IBOutlet UIWebView *webView;
//    IBOutlet UIView *view;
	IBOutlet UIToolbar *toolBar;
	IBOutlet UIActivityIndicatorView *activityIndicator;
	IBOutlet UIBarButtonItem *backButton,*forwardButton;
	IBOutlet UITextField *addressTestField;
	IBOutlet DownloadViewController *downloadViewController;
	IBOutlet DetailViewControllerIphone *detailViewController;
	
	IBOutlet UIView *infoDownloadView;
	IBOutlet UILabel *infoDownloadLbl;
	NSString *home;
	NSString *custom_URL[MAX_CUSTOM_URL];
	NSString *custom_URL_name[MAX_CUSTOM_URL];
	int custom_url_count;
	
}

@property (nonatomic,retain) IBOutlet UIView *infoDownloadView;//,*view;
@property (nonatomic,retain) IBOutlet UILabel *infoDownloadLbl;
@property (nonatomic,retain) IBOutlet UIWebView *webView;
@property (nonatomic,retain) IBOutlet UIToolbar *toolBar;
@property (nonatomic,retain) IBOutlet UIActivityIndicatorView *activityIndicator;
@property (nonatomic,retain) IBOutlet UIBarButtonItem *backButton,*forwardButton;
@property (nonatomic,retain) IBOutlet UITextField *addressTestField;
@property (nonatomic,retain) IBOutlet DownloadViewController *downloadViewController;
@property (nonatomic,retain) IBOutlet DetailViewControllerIphone *detailViewController;

-(IBAction) goPlayer;
-(IBAction) goBack:(id)sender;
-(IBAction) goForward:(id)sender;
-(IBAction) newBookmark:(id)sender;
-(IBAction) goBookmarks;
-(IBAction) goHome:(id)sender;
-(IBAction) newUrlEntered:(id)sender;
-(IBAction) stopLoading:(id)sender;
-(IBAction) refresh:(id)sender;

-(void)goToURL:(NSString*)address;
-(void)goToURLwithLoad:(NSString*)address;
-(void)loadWorldCharts;
-(void)loadUserGuide;
-(void)loadLastURL;
-(void)loadHome;
-(void) deleteBookmark:(int)index;
-(void) saveBookmarks;
-(void) loadBookmarks;
-(void) openPopup:(NSString *)fileToDownload;
-(void) closePopup;
@end
