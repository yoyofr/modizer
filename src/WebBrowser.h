//
//  WebBrowser.h
//  modizer4
//
//  Created by yoyofr on 7/4/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>
#import "DownloadViewController.h"
#import "DetailViewControllerIphone.h"
#import "ModizerWebView.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"

#define MAX_CUSTOM_URL 256

@class DownloadViewController;
@class DetailViewControllerIphone;

@interface WebBrowser : UIViewController <UINavigationControllerDelegate,WKNavigationDelegate,WKUIDelegate,UITextFieldDelegate> {
	ModizerWebView *webView;
    IBOutlet UIView *view;
	IBOutlet UIToolbar *toolBar;
	IBOutlet UIProgressView *progressIndicator;
	IBOutlet UIBarButtonItem *backButton,*forwardButton;
	IBOutlet UITextField *addressTestField;
	IBOutlet DownloadViewController *downloadViewController;
	IBOutlet DetailViewControllerIphone *detailViewController;
    
    WaitingView *waitingView,*waitingViewPlayer;
    NSTimer *repeatingTimer;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    bool darkMode;
    bool forceReloadCells;
	
	IBOutlet UIView *infoDownloadView;
	IBOutlet UILabel *infoDownloadLbl;
	NSString *home;
	
    NSLayoutConstraint *bottomConstraint,*topConstraint;
    
    NSMutableArray *custom_URL;
    NSMutableArray *custom_URL_name;
    int custom_url_count;
}

@property (strong) NSMutableArray *custom_URL;
@property (strong) NSMutableArray *custom_URL_name;
@property int custom_url_count;

@property (nonatomic,retain) IBOutlet UIView *infoDownloadView;
@property (nonatomic,retain) IBOutlet UILabel *infoDownloadLbl;
@property (nonatomic,strong) ModizerWebView *webView;
@property (nonatomic,retain) IBOutlet UIToolbar *toolBar;
@property (nonatomic,retain) IBOutlet UIProgressView *progressIndicator;
@property (nonatomic,retain) IBOutlet UIBarButtonItem *backButton,*forwardButton;
@property (nonatomic,retain) IBOutlet UITextField *addressTestField;
@property (nonatomic,retain) IBOutlet DownloadViewController *downloadViewController;
@property (nonatomic,retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) WaitingView *waitingView,*waitingViewPlayer;

-(IBAction) goPlayer;
-(void) updateMiniPlayer;
-(void) refreshMiniplayer;

-(IBAction) goBackRootVC:(id)sender;
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
