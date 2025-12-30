//
//  RootViewControllerXPWebParser.mm
//  modizer
//
//  Created by Yohann Magnien on 10/03/24.
//  Copyright __YoyoFR / Yohann Magnien__ 2024. All rights reserved.
//


extern void *LoadingProgressObserverContext;

#import "RootViewControllerXPWebParser.h"
#import "ModizFileHelper.h"

@implementation RootViewControllerXPWebParser
@synthesize repeatingTimer;
@synthesize mFileMngr;
@synthesize detailViewController;
@synthesize downloadViewController;
@synthesize tableView,sBar;
@synthesize childController;
@synthesize mSearchText;
@synthesize popTipView;
@synthesize mWebBaseURL,rootDir;

#pragma mark -
#pragma mark Alert functions

#import "AlertsCommonFunctions.h"

#pragma mark -
#pragma mark Miniplayer
#include "MiniPlayerImplementTableView.h"

#pragma mark -
#pragma mark Search functions
#import "SearchCommonFunctions.h"

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
        [self updateMiniPlayer];
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////
#define HAS_DETAILVIEW_CONT
#include "PlaylistCommonFunctions.h"

-(void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer {
    CGPoint p = [gestureRecognizer locationInView:self.tableView];
    
    NSIndexPath *indexPath = [self.tableView indexPathForRowAtPoint:p];
    {
        if (indexPath != nil) {
            if ((gestureRecognizer.state==UIGestureRecognizerStateBegan)||(gestureRecognizer.state==UIGestureRecognizerStateChanged)) {
                int crow=indexPath.row;
                
                    //display popup
                    t_WEB_browse_entry *cur_db_entries;
                    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
                    
                    NSString *str=cur_db_entries[crow].fullpath;
                    if (self.popTipView == nil) {
                        self.popTipView = [[CMPopTipView alloc] initWithMessage:str];
                        self.popTipView.delegate = self;
                        self.popTipView.backgroundColor = [UIColor lightGrayColor];
                        self.popTipView.textColor = [UIColor darkTextColor];
                        
                        [self.popTipView presentPointingAtView:[self.tableView cellForRowAtIndexPath:indexPath] inView:self.tableView animated:YES];
                        popTipViewRow=crow;
                        popTipViewSection=0;
                    } else {
                        if ((popTipViewRow!=crow)||(popTipViewSection!=0)||([str compare:self.popTipView.message]!=NSOrderedSame)) {
                            self.popTipView.message=str;
                            [self.popTipView presentPointingAtView:[self.tableView cellForRowAtIndexPath:indexPath] inView:self.tableView animated:YES];
                            popTipViewRow=crow;
                            popTipViewSection=0;
                        }
                    }
            } else {
                //hide popup
                if (popTipView!=nil) {
                    [self.popTipView dismissAnimated:YES];
                    popTipView=nil;
                }
            }
        }
    }
}

#pragma mark CMPopTipViewDelegate methods
- (void)popTipViewWasDismissedByUser:(CMPopTipView *)_popTipView {
    // User can tap CMPopTipView to dismiss it
    self.popTipView = nil;
}

- (id)findChildOfClass:(Class)cls inTabBarController:(UITabBarController *)tbc {
    for (UIViewController *vc in tbc.viewControllers) {
        // Unwrap nav controllers if present
        UIViewController *candidate = vc;
        if ([vc isKindOfClass:[UINavigationController class]]) {
            candidate = ((UINavigationController *)vc).viewControllers.firstObject;
        }
        if ([candidate isKindOfClass:cls]) {
            return candidate;
        }
    }
    return nil;
}

-(void) loadControllers {
    // With automatic storyboard loading, the window and root VC are created by UIKit.
    UIWindow *window=[UIApplication sharedApplication].windows.firstObject;
    if (!window) {
        // Fallback to keyWindow if needed
        window = [UIApplication sharedApplication].keyWindow;
    }
    UITabBarController *tbc = (UITabBarController *)window.rootViewController;
    if (![tbc isKindOfClass:[UITabBarController class]]) {
        MDZELog("[SceneDelegate] Unexpected root VC: %@", NSStringFromClass([window.rootViewController class]));
        return;
    }
    // Resolve specific child controllers
    if (!self.downloadViewController) self.downloadViewController = [self findChildOfClass:[DownloadViewController class] inTabBarController:tbc];
    if (!self.detailViewController) self.detailViewController = [self findChildOfClass:[DetailViewControllerIphone class] inTabBarController:tbc];
}


- (void)viewDidLoad {
    START_PROFILE
    childController=NULL;
    
    [self loadControllers];
    
    dictActionBtn=[NSMutableDictionary dictionaryWithCapacity:64];
    
    mPopupAnimation=0;
    htmlData=nil;
    sort_mode=0;
    
    self.navigationController.delegate = self;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    mFileMngr=[[NSFileManager alloc] init];
    
    imagesCache = [[ImagesCache alloc] init];
    
    navbarTitle=[[UILabel alloc] init];
    navbarTitle.userInteractionEnabled=TRUE;
    
    if (browse_depth>0) {
        self.navigationItem.titleView=navbarTitle;
        
        navbarTitle.text=self.title;
        self.navigationItem.title=navbarTitle.text;
        
    }
    
    ratingImg[0] = @"heart-empty.png";
    ratingImg[1] = @"heart-half-filled.png";
    ratingImg[2] = @"heart-filled.png";
    
    /* Init popup view*/
    /**/
    
    //self.tableView.pagingEnabled;
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.tableView.sectionHeaderHeight = 18;
    //self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
    self.tableView.rowHeight = 40;
    //self.tableView.backgroundColor = [UIColor clearColor];
    //    self.tableView.backgroundColor = [UIColor blackColor];
    
    popTipViewRow=-1;
    popTipViewSection=-1;
    UILongPressGestureRecognizer *lpgr = [[UILongPressGestureRecognizer alloc]
                                          initWithTarget:self action:@selector(handleLongPress:)];
    lpgr.minimumPressDuration = 1.0; //seconds
    lpgr.delegate = self;
    [self.tableView addGestureRecognizer:lpgr];
    //[lpgr release];
    
    shouldFillKeys=1;
    mSearch=0;
    
    search_dbWEB=0;  //reset to ensure search_dbWEB is not used by default
    
    
    dbWEB_entries=NULL;
    search_dbWEB_entries=NULL;
    
    dbWEB_nb_entries=0;
    search_dbWEB_nb_entries=0;
    
    search_dbWEB_hasFiles=0;
    dbWEB_hasFiles=0;
    
    mSearchText=nil;
    mClickedPrimAction=0;
    
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:NOW_PLAYING_ICON] style:UIBarButtonItemStylePlain target:self action:@selector(goPlayer)];
    self.navigationItem.rightBarButtonItem = item;
    
    /////////////////////////////////////
    // Waiting view
    /////////////////////////////////////
    waitingView = [[WaitingView alloc] init];
    waitingView.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingView];
    //waitingView.hidden=TRUE;
    
    NSDictionary *views = NSDictionaryOfVariableBindings(waitingView);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingView(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingView(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    waitingViewPlayer = [[WaitingView alloc] init];
    waitingViewPlayer.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingViewPlayer];
    waitingViewPlayer.hidden=TRUE;
    
    views = NSDictionaryOfVariableBindings(waitingViewPlayer);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    [super viewDidLoad];

END_PROFILE
}

-(void) createHTMLWebViewIfNeeded {
    if (!_htmlWebView) {
        WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];

        // Set minimum font size to ensure readability
        config.preferences.minimumFontSize = 9.0;

        _htmlWebView = [[WKWebView alloc] initWithFrame:CGRectZero configuration:config];
        _htmlWebView.translatesAutoresizingMaskIntoConstraints = NO;
        _htmlWebView.navigationDelegate = self;

        // Enable scrolling with visible scrollbars
        _htmlWebView.scrollView.showsHorizontalScrollIndicator = YES;
        _htmlWebView.scrollView.showsVerticalScrollIndicator = YES;

        if (darkMode) {
            _htmlWebView.backgroundColor = [UIColor blackColor];
        } else {
            _htmlWebView.backgroundColor = [UIColor whiteColor];
        }
    }
}

-(NSString*) fixWindowsCharacters:(NSString*)input {
    if (!input || [input length] == 0) return input;

    NSMutableString *result = [NSMutableString stringWithString:input];

    // Replace Windows-1252 characters that don't have valid UTF-8 equivalents
    // These are characters in the range 0x80-0x9F that are problematic

    // 0x95 - Bullet point (&#149;)
    unichar bullet = 0x95;
    NSString *bulletStr = [NSString stringWithCharacters:&bullet length:1];
    [result replaceOccurrencesOfString:bulletStr withString:@"•" options:NSLiteralSearch range:NSMakeRange(0, [result length])];

    // 0x96 - En dash (&#150;)
    unichar endash = 0x96;
    NSString *endashStr = [NSString stringWithCharacters:&endash length:1];
    [result replaceOccurrencesOfString:endashStr withString:@"–" options:NSLiteralSearch range:NSMakeRange(0, [result length])];

    // 0x97 - Em dash (&#151;)
    unichar emdash = 0x97;
    NSString *emdashStr = [NSString stringWithCharacters:&emdash length:1];
    [result replaceOccurrencesOfString:emdashStr withString:@"—" options:NSLiteralSearch range:NSMakeRange(0, [result length])];

    // 0x91, 0x92 - Left/right single quotes (&#145;, &#146;)
    unichar lsquote = 0x91;
    unichar rsquote = 0x92;
    [result replaceOccurrencesOfString:[NSString stringWithCharacters:&lsquote length:1] withString:@"'" options:NSLiteralSearch range:NSMakeRange(0, [result length])];
    [result replaceOccurrencesOfString:[NSString stringWithCharacters:&rsquote length:1] withString:@"'" options:NSLiteralSearch range:NSMakeRange(0, [result length])];

    // 0x93, 0x94 - Left/right double quotes (&#147;, &#148;)
    unichar ldquote = 0x93;
    unichar rdquote = 0x94;
    unichar ldqChar = 0x201C; // Unicode left double quote
    unichar rdqChar = 0x201D; // Unicode right double quote
    [result replaceOccurrencesOfString:[NSString stringWithCharacters:&ldquote length:1] withString:[NSString stringWithCharacters:&ldqChar length:1] options:NSLiteralSearch range:NSMakeRange(0, [result length])];
    [result replaceOccurrencesOfString:[NSString stringWithCharacters:&rdquote length:1] withString:[NSString stringWithCharacters:&rdqChar length:1] options:NSLiteralSearch range:NSMakeRange(0, [result length])];

    // 0x85 - Ellipsis (&#133;)
    unichar ellipsis = 0x85;
    [result replaceOccurrencesOfString:[NSString stringWithCharacters:&ellipsis length:1] withString:@"…" options:NSLiteralSearch range:NSMakeRange(0, [result length])];

    return result;
}

-(BOOL) shouldDisplayHTMLContent {
    return (htmlData != nil && [htmlData length] > 0);
}

-(void) switchToHTMLView {
    [self createHTMLWebViewIfNeeded];

    if (_htmlWebView.superview == nil) {
        // Remove tableView
        [tableView removeFromSuperview];

        // Add WKWebView directly to main view
        [self.view addSubview:_htmlWebView];

        // Simple constraints - WKWebView takes full view size
        [_htmlWebView.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor].active = YES;
        [_htmlWebView.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor].active = YES;

        if (sBar && !sBar.hidden) {
            [_htmlWebView.topAnchor constraintEqualToAnchor:sBar.bottomAnchor].active = YES;
        } else {
            [_htmlWebView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor].active = YES;
        }

        [_htmlWebView.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor].active = YES;
    }

    // Fix Windows-1252 characters that were decoded by TFHpple
    NSString *cleanedData = [self fixWindowsCharacters:htmlData];

    // Check if htmlData is already a complete HTML document
    BOOL isCompleteHTML = ([cleanedData rangeOfString:@"<html" options:NSCaseInsensitiveSearch].location != NSNotFound) ||
                          ([cleanedData rangeOfString:@"<!DOCTYPE html" options:NSCaseInsensitiveSearch].location != NSNotFound);

    NSString *fullHTML;
    if (isCompleteHTML) {
        // Replace viewport to use fixed minimum width (like a desktop browser)
        // This ensures content is rendered at a minimum readable size with horizontal scroll if needed
        NSError *error = nil;
        NSRegularExpression *viewportRegex = [NSRegularExpression regularExpressionWithPattern:@"<meta[^>]*name=['\"]viewport['\"][^>]*>"
                                                                                        options:NSRegularExpressionCaseInsensitive
                                                                                          error:&error];
        NSUInteger matches = [viewportRegex numberOfMatchesInString:cleanedData options:0 range:NSMakeRange(0, [cleanedData length])];

        if (!error && matches > 0) {
            // Replace existing viewport with fixed width and initial scale
            NSString *fixedViewport = @"<meta name='viewport' content='width=600, initial-scale=1.0, user-scalable=yes, shrink-to-fit=no'>";
            fullHTML = [viewportRegex stringByReplacingMatchesInString:cleanedData
                                                                options:0
                                                                  range:NSMakeRange(0, [cleanedData length])
                                                           withTemplate:fixedViewport];
            NSLog(@"Replaced viewport meta tag (found %lu matches)", (unsigned long)matches);
        } else {
            // No viewport found, inject one after <head>
            NSRegularExpression *headRegex = [NSRegularExpression regularExpressionWithPattern:@"<head[^>]*>"
                                                                                        options:NSRegularExpressionCaseInsensitive
                                                                                          error:&error];
            if (!error && [headRegex numberOfMatchesInString:cleanedData options:0 range:NSMakeRange(0, [cleanedData length])] > 0) {
                NSString *headWithViewport = @"$0<meta name='viewport' content='width=600, initial-scale=1.0, user-scalable=yes, shrink-to-fit=no'>";
                fullHTML = [headRegex stringByReplacingMatchesInString:cleanedData
                                                                options:0
                                                                  range:NSMakeRange(0, [cleanedData length])
                                                           withTemplate:headWithViewport];
                NSLog(@"Injected viewport meta tag after <head>");
            } else {
                fullHTML = cleanedData;
                NSLog(@"No viewport and no <head> found, using HTML as-is");
            }
        }

        // Also inject CSS to force minimum content width (important for Mac Catalyst)
        NSRegularExpression *headCloseRegex = [NSRegularExpression regularExpressionWithPattern:@"</head>"
                                                                                        options:NSRegularExpressionCaseInsensitive
                                                                                          error:&error];
        if (!error && [headCloseRegex numberOfMatchesInString:fullHTML options:0 range:NSMakeRange(0, [fullHTML length])] > 0) {
            NSString *minWidthCSS = @"<style>html, body { min-width: 600px !important; overflow-x: auto !important; } body > * { min-width: 600px !important; }</style></head>";
            fullHTML = [headCloseRegex stringByReplacingMatchesInString:fullHTML
                                                                options:0
                                                                  range:NSMakeRange(0, [fullHTML length])
                                                           withTemplate:minWidthCSS];
            NSLog(@"Injected min-width CSS");
        }
    } else {
        // Wrap content in HTML template with fixed width viewport
        NSString *htmlTemplate = @"<html><head><meta charset='UTF-8'><meta name='viewport' content='width=600, initial-scale=1.0, user-scalable=yes, shrink-to-fit=no'><style>body { font-family: -apple-system; font-size: 9pt; -webkit-text-size-adjust: none; margin: 16px; %@ }</style></head><body>%@</body></html>";

        NSString *colorStyle;
        if (darkMode) {
            colorStyle = @"background-color: #000; color: #fff;";
        } else {
            colorStyle = @"background-color: #fff; color: #000;";
        }

        fullHTML = [NSString stringWithFormat:htmlTemplate, colorStyle, cleanedData];
    }

    [_htmlWebView loadHTMLString:fullHTML baseURL:nil];
}

-(void) switchToTableView {
    if (_htmlWebView && _htmlWebView.superview) {
        [_htmlWebView removeFromSuperview];
    }

    if (tableView.superview == nil) {
        [self.view addSubview:tableView];
    }

    [tableView reloadData];
    [tableView layoutIfNeeded];
}

-(void) fillKeysCompleted {
    //called when fillKeys has finished

    if ([self shouldDisplayHTMLContent]) {
        // Don't hide waiting yet - will hide when webview finishes loading
        [self switchToHTMLView];
    } else {
        [self hideWaiting];
        [self switchToTableView];
    }

    fillKeysInProgress=0;
}

-(void) fillKeys {
    //to be implemented in each parser
    
    //call fillKeyCompleted at the end
    dispatch_async(dispatch_get_main_queue(), ^(void){
        [self fillKeysCompleted];
    });
}

-(void) traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];

    // Update htmlWebView colors if it exists
    if (_htmlWebView) {
        if (darkMode) {
            _htmlWebView.backgroundColor = [UIColor blackColor];
        } else {
            _htmlWebView.backgroundColor = [UIColor whiteColor];
        }
        // Reload HTML with updated colors if currently displayed
        if ([self shouldDisplayHTMLContent] && _htmlWebView.superview) {
            [self switchToHTMLView];
        }
    }

    [self.tableView reloadData];
    [self.tableView layoutIfNeeded];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleDefault;
}

-(void) viewWillAppear:(BOOL)animated {
    //    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    [self.sBar setBarStyle:UIBarStyleDefault];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    
    self.navigationController.delegate = self;
    
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    
    if ([detailViewController mPlaylist_size]>0) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    } else {
        wasMiniPlayerOn=false;
        [self hideMiniPlayer];
    }
    
    if (childController) {
        //[childController release];
        childController = NULL;
    }
    
    //Reset rating if applicable (ensure updated value)
    if (dbWEB_nb_entries) {
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].rating=-1;
        }
    }
    if (search_dbWEB_nb_entries) {
        for (int i=0;i<search_dbWEB_nb_entries;i++) {
            search_dbWEB_entries_data[i].rating=-1;
        }
    }
    /////////////
    
    [waitingViewPlayer resetCancelStatus];
    waitingViewPlayer.hidden=detailViewController.waitingView.hidden;
    waitingViewPlayer.btnStopCurrentAction.hidden=detailViewController.waitingView.btnStopCurrentAction.hidden;
    waitingViewPlayer.progressView.progress=detailViewController.waitingView.progressView.progress;
    waitingViewPlayer.progressView.hidden=detailViewController.waitingView.progressView.hidden;
    waitingViewPlayer.lblTitle.text=[NSString stringWithString:detailViewController.waitingView.lblTitle.text];
    waitingViewPlayer.lblDetail.text=[NSString stringWithString:detailViewController.waitingView.lblDetail.text];
    
    //    [waitingViewPlayer.progressView setObservedProgress:detailViewController.mplayer.extractProgress];
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView addObserver:self
                                       forKeyPath:observedSelector
                                          options:NSKeyValueObservingOptionInitial
                                          context:LoadingProgressObserverContext];
    
    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.20f target:self selector:@selector(updateLoadingInfos:) userInfo:nil repeats: YES]; //5 times/second
    
    self.mdzChangeObserverToken =
    [[NSNotificationCenter defaultCenter] addObserverForName:MDZFileStatsChangedNotification
                                                      object:nil
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification * _Nonnull note) {
        // Consommer la notification
        NSDictionary *info = note.userInfo;
        //        NSString *fileName = info[@"fileName"];
        //        NSString *filePath = info[@"filePath"];
        // Mets à jour l’UI / ton modèle
        //self.forceReloadCells=true;
        [self fillKeys];
        [self.tableView reloadData];
        [self.tableView layoutIfNeeded];
    }];
    
    [super viewWillAppear:animated];
    
}
-(void) refreshViewAfterDownload {
    if (childController) [(RootViewControllerXPWebParser*)childController refreshViewAfterDownload];
    else {
        //will trigger a background task
        dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
            [self fillKeys];
        });
    }
}

- (void)checkCreate:(NSString *)filePath {
    //NSString *completePath=[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getAppHomeDirectory],filePath];
    NSError *err;
    [mFileMngr createDirectoryAtPath:filePath withIntermediateDirectories:TRUE attributes:nil error:&err];
}

- (void)viewDidAppear:(BOOL)animated {
    [self hideWaiting];
    
    [super viewDidAppear:animated];
    
    if (shouldFillKeys) {
        
        [self updateWaitingTitle:NSLocalizedString(@"Loading & parsing",@"")];
        [self updateWaitingDetail:@""];
        [self showWaiting];
        [self flushMainLoop];
        
        dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
            [self fillKeys];
        });
    } else {
        dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
            [self fillKeys];
        });
    }
    
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}

- (void)viewDidDisappear:(BOOL)animated {
    [self hideWaiting];
    [repeatingTimer invalidate];
    repeatingTimer = nil;

    [self.searchDebounceTimer invalidate];
    self.searchDebounceTimer = nil;

    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView removeObserver:self
                                          forKeyPath:observedSelector
                                             context:LoadingProgressObserverContext];

    if (self.mdzChangeObserverToken) {
        [[NSNotificationCenter defaultCenter] removeObserver:self.mdzChangeObserverToken];
        self.mdzChangeObserverToken = nil;
    }
    [super viewDidDisappear:animated];
}

#pragma mark - WKNavigationDelegate

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
    // Hide waiting indicator when HTML content is fully loaded
    [self hideWaiting];

//    // Force minimum contentSize width
//    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
//        CGSize currentContentSize = webView.scrollView.contentSize;
//        CGFloat minWidth = 600.0;
//
//        if (currentContentSize.width < minWidth) {
//            // Force contentSize to have minimum width
//            webView.scrollView.contentSize = CGSizeMake(minWidth, currentContentSize.height);
//            NSLog(@"Forced contentSize width from %.0f to %.0f", currentContentSize.width, minWidth);
//        }
//
//        NSLog(@"Final contentSize: %@, frame: %@",
//              NSStringFromCGSize(webView.scrollView.contentSize),
//              NSStringFromCGSize(webView.frame.size));
//    });
}

- (void)webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error {
    // Hide waiting indicator even on error
    [self hideWaiting];
    NSLog(@"WebView navigation failed: %@", error);
}

#pragma mark - View Transitions

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [self.tableView reloadData];
    [tableView layoutIfNeeded];
    [miniplayerVC viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}


- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
    [tableView reloadData];
    [tableView layoutIfNeeded];
}

// Ensure that the view controller supports rotation and that the split view can therefore show in both portrait and landscape.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    [tableView reloadData];
    [tableView layoutIfNeeded];
    return YES;
}

#pragma mark -
#pragma mark Table view data source

- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    return nil;
}


- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return nil;
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    //Check if "Get all entries" has to be displayed
    if (search_dbWEB) {
        return search_dbWEB_entries_count;
    } else {
        return dbWEB_entries_count;
    }
}

- (NSInteger)tableView:(UITableView *)tabView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    return -1;
}

// Override to support editing the table view.
//- (void)tableView:(UITableView *)tabView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
//    
//    if (editingStyle == UITableViewCellEditingStyleDelete) {
//        // Delete the row from the data source
//        //delete entry
//        t_WEB_browse_entry *cur_db_entries;
//        cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
//        
//        //delete file
//        NSString *fullpath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",cur_db_entries[indexPath.row].fullpath];
//        NSError *err;
//        DBHelper::deleteStatsFileDB(fullpath);
//        cur_db_entries[indexPath.row].downloaded=0;
//        //delete local file
//        [mFileMngr removeItemAtPath:fullpath error:&err];
//        //ask for a reload/redraw
//        [tabView reloadData];
//        
//    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
//        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
//    }
//}

- (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    t_WEB_browse_entry *cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    if (cur_db_entries) {
        if (cur_db_entries[indexPath.row].downloaded==1) return YES;
    }
    return NO;
}

- (UISwipeActionsConfiguration *)tableView:(UITableView *)tableView
trailingSwipeActionsConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath {

//    t_WEB_browse_entry *cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);

    // Only show actions for downloaded files
//    if (cur_db_entries[indexPath.row].downloaded != 1) {
//        return nil;
//    }

    // DELETE ACTION
    UIContextualAction *deleteAction =
    [UIContextualAction contextualActionWithStyle:UIContextualActionStyleDestructive
                                            title:NSLocalizedString(@"Delete", @"")
                                          handler:^(UIContextualAction *action,
                                                    UIView *sourceView,
                                                    void (^completionHandler)(BOOL)) {

        t_WEB_browse_entry *cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);

        //delete file
        NSString *fullpath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",cur_db_entries[indexPath.row].fullpath];
        NSError *err;
        DBHelper::deleteStatsFileDB(fullpath);
        cur_db_entries[indexPath.row].downloaded=0;
        cur_db_entries[indexPath.row].rating=-1;
        cur_db_entries[indexPath.row].playcount=-1;

        //delete local file
        [mFileMngr removeItemAtPath:fullpath error:&err];

        // Reload the cell to show the file is no longer downloaded
        [tableView reloadRowsAtIndexPaths:@[indexPath]
                         withRowAnimation:UITableViewRowAnimationAutomatic];

        completionHandler(YES);
    }];
    deleteAction.backgroundColor = [UIColor redColor];

    // Return multiple actions - they appear from right to left
    return [UISwipeActionsConfiguration configurationWithActions:@[deleteAction]];
}


#pragma mark UISearchBarDelegate
- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar {
    // only show the status bar’s cancel button while in edit mode
    sBar.showsCancelButton = YES;
    sBar.autocorrectionType = UITextAutocorrectionTypeNo;
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    
    // flush the previous search content
    //[tableData removeAllObjects];
}
- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar {
    //[self fillKeys];
    //[tableView reloadData];
    //mSearch=0;
    sBar.showsCancelButton = NO;
}
- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
    //if (mSearchText) [mSearchText release];
    
    mSearchText=[[NSString alloc] initWithString:searchText];
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    if (mSearch) shouldFillKeys=1;
    search_dbWEB=0;
//    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
//        [self fillKeys];
//    });
    
    // Cancel previous search timer to debounce
    [self.searchDebounceTimer invalidate];
    self.searchDebounceTimer = nil;

    // Schedule new search after delay
    self.searchDebounceTimer = [NSTimer scheduledTimerWithTimeInterval:0.5
                                                                 target:self
                                                               selector:@selector(fillKeys)
                                                               userInfo:nil
                                                                repeats:NO];
}
- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
    //if (mSearchText) [mSearchText release];
    mSearchText=nil;
    sBar.text=nil;
    mSearch=0;
    sBar.showsCancelButton = NO;
    [searchBar resignFirstResponder];
    //shouldFillKeys=1;
    search_dbWEB=0;
//    dispatch_async(dispatch_get_main_queue(), ^(void){
//        [self fillKeys];
//    });
    // Cancel previous search timer to debounce
    [self.searchDebounceTimer invalidate];
    self.searchDebounceTimer = nil;

    // Schedule new search after delay
    self.searchDebounceTimer = [NSTimer scheduledTimerWithTimeInterval:0.5
                                                                 target:self
                                                               selector:@selector(fillKeys)
                                                               userInfo:nil
                                                                repeats:NO];
}
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar {
    [searchBar resignFirstResponder];
}

- (void)moveCursorOnce {
    UITextField *tf = self.sBar.searchTextField;
    UITextPosition *pos = tf.selectedTextRange.start;

    NSInteger offset = (self.activeKey == UIKeyboardHIDUsageKeyboardLeftArrow) ? -1 : 1;
    UITextPosition *newPos = [tf positionFromPosition:pos offset:offset];

    if (newPos) {
        tf.selectedTextRange = [tf textRangeFromPosition:newPos toPosition:newPos];
    }
}


- (void)startRepeating {
    [self.repeatTimer invalidate];
    self.repeatTimer = [NSTimer scheduledTimerWithTimeInterval:0.05
                                                        target:self
                                                      selector:@selector(moveCursorOnce)
                                                      userInfo:nil
                                                       repeats:YES];
}


- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    for (UIPress *press in presses) {
        UIKey *key = press.key;
        if (!key) continue;

        if (key.keyCode == UIKeyboardHIDUsageKeyboardLeftArrow ||
            key.keyCode == UIKeyboardHIDUsageKeyboardRightArrow) {

            self.activeKey = key.keyCode;
            [self moveCursorOnce];

            // délai initial macOS (~0.45s)
            self.repeatTimer = [NSTimer scheduledTimerWithTimeInterval:0.45
                                                                target:self
                                                              selector:@selector(startRepeating)
                                                              userInfo:nil
                                                               repeats:NO];
            return;
        }
    }
    [super pressesBegan:presses withEvent:event];
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    [self.repeatTimer invalidate];
    self.repeatTimer = nil;
    self.activeKey = 0;
    [super pressesEnded:presses withEvent:event];
}

- (void)pressesCancelled:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    [self.repeatTimer invalidate];
    self.repeatTimer = nil;
    self.activeKey = 0;
    [super pressesCancelled:presses withEvent:event];
}


-(IBAction)goPlayer {
    if (detailViewController.mPlaylist_size) {
        if (detailViewController) {
            @try {
                [self.navigationController pushViewController:detailViewController animated:YES];
            } @catch (NSException * ex) {
                //“Pushing the same view controller instance more than once is not supported”
                //NSInvalidArgumentException
                MDZELog("Exception: [%@]:%@",[ex  class], ex );
                MDZELog("ex.name:'%@'", ex.name);
                MDZELog("ex.reason:'%@'", ex.reason);
                //Full error includes class pointer address so only care if it starts with this error
                NSRange range = [ex.reason rangeOfString:@"Pushing the same view controller instance more than once is not supported"];
                
                if ([ex.name isEqualToString:@"NSInvalidArgumentException"] &&
                    range.location != NSNotFound) {
                    //view controller already exists in the stack - just pop back to it
                    [self.navigationController popToViewController:detailViewController animated:YES];
                } else {
                    MDZELog("ERROR:UNHANDLED EXCEPTION TYPE:%@", ex);
                }
            } @finally {
            }
        } else {
            MDZELog("ERROR:pushViewController: viewController is nil");
        }
    }
    else {
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"")];
    }
}

#pragma mark -
#pragma mark Table view delegate

- (void) primaryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    {
        t_WEB_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
        
        if (cur_db_entries[indexPath.row].isFile) { //FILE
            //File selected, start download is needed
            NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",cur_db_entries[indexPath.row].fullpath];
            
            if (cur_db_entries[indexPath.row].downloaded==1) {
                NSMutableArray *array_label = [[NSMutableArray alloc] init];
                NSMutableArray *array_path = [[NSMutableArray alloc] init];
                [array_label addObject:cur_db_entries[indexPath.row].label];
                [array_path addObject:cur_db_entries[indexPath.row].fullpath];
                cur_db_entries[indexPath.row].rating=-1;
                [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                
                [tableView reloadData];
                [tableView layoutIfNeeded];
            } else {
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                
                [downloadViewController addURLToDownloadList:cur_db_entries[indexPath.row].URL fileName:cur_db_entries[indexPath.row].label filePath:cur_db_entries[indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:1];
                
            }
        }
        
    }
    
    [self hideWaiting];
    
    
}
- (void) secondaryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    t_WEB_browse_entry *cur_db_entries;
    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    
    if (cur_db_entries[indexPath.row].isFile) { //FILE
        //File selected, start download is needed
        NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",cur_db_entries[indexPath.row].fullpath];
        mClickedPrimAction=2;
        
        if (cur_db_entries[indexPath.row].downloaded==1) {
            //add to playlist
            [self addToPlaylistSelView:cur_db_entries[indexPath.row].fullpath label:cur_db_entries[indexPath.row].label showNowListening:true];
            
            cur_db_entries[indexPath.row].rating=-1;
            [tableView reloadData];
            [tableView layoutIfNeeded];
        } else {
            [self checkCreate:[localPath stringByDeletingLastPathComponent]];
            
            [downloadViewController addURLToDownloadList:cur_db_entries[indexPath.row].URL fileName:cur_db_entries[indexPath.row].label filePath:cur_db_entries[indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
        }
    }
    [self hideWaiting];
}

- (void) accessoryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    mAccessoryButton=1;
    [self tableView:tableView didSelectRowAtIndexPath:indexPath];
}

- (void)tableView:(UITableView *)tabView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    t_WEB_browse_entry *cur_db_entries;
    cur_db_entries=(search_dbWEB?search_dbWEB_entries:dbWEB_entries);
    
    if (cur_db_entries[indexPath.row].isFile) { //FILE
        //File selected, start download is needed
        NSString *localPath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@",cur_db_entries[indexPath.row].fullpath];
        mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
        
        if (cur_db_entries[indexPath.row].downloaded==1) {
            if (mClickedPrimAction) {
                NSMutableArray *array_label = [[NSMutableArray alloc] init];
                NSMutableArray *array_path = [[NSMutableArray alloc] init];
                [array_label addObject:cur_db_entries[indexPath.row].label];
                [array_path addObject:cur_db_entries[indexPath.row].fullpath];
                cur_db_entries[indexPath.row].rating=-1;
                [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                
                [tabView reloadData];
                [tableView layoutIfNeeded];
            } else {
                if ([detailViewController add_to_playlist:localPath fileName:cur_db_entries[indexPath.row].label forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)]) {
                    if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                    
                    cur_db_entries[indexPath.row].rating=-1;
                    [tabView reloadData];
                    [tableView layoutIfNeeded];
                }
            }
        } else {
            [self checkCreate:[localPath stringByDeletingLastPathComponent]];
            
            
            [downloadViewController addURLToDownloadList:cur_db_entries[indexPath.row].URL fileName:cur_db_entries[indexPath.row].label filePath:cur_db_entries[indexPath.row].fullpath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
            
        }
    } else {
        childController = [[RootViewControllerXPWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
        //set new title
        childController.title = cur_db_entries[indexPath.row].fullpath;
        // Set new directory
        ((RootViewControllerXPWebParser*)childController)->browse_depth = browse_depth+1;
        ((RootViewControllerXPWebParser*)childController)->detailViewController=detailViewController;
        ((RootViewControllerXPWebParser*)childController)->downloadViewController=downloadViewController;
        ((RootViewControllerXPWebParser*)childController)->mWebBaseURL=cur_db_entries[indexPath.row].URL;
        
        //childController.view.frame=self.view.frame;
        // Ensure proper layout under navigation/tab bars
        if ([childController respondsToSelector:@selector(setEdgesForExtendedLayout:)]) {
            childController.edgesForExtendedLayout = UIRectEdgeNone;
            childController.extendedLayoutIncludesOpaqueBars = NO;
        }
        if ([childController isKindOfClass:[UITableViewController class]]) {
            ((UITableViewController *)childController).tableView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentAutomatic;
        } else if ([childController.view isKindOfClass:[UIScrollView class]]) {
            ((UIScrollView *)childController.view).contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentAutomatic;
        }
        
        // And push the window
        [self.navigationController pushViewController:childController animated:YES];
    }
}


#pragma mark -
#pragma mark popup functions

-(void) hidePopup {
    infoMsgView.hidden=YES;
    mPopupAnimation=0;
}

-(void) openPopup:(NSString *)msg {
    CGRect frame;
    if (mPopupAnimation) return;
    mPopupAnimation=1;
    infoMsgView.layer.zPosition=MAXFLOAT;
    frame=infoMsgView.frame;
    frame.origin.y=self.view.frame.size.height;
    infoMsgView.frame=frame;
    infoMsgView.hidden=NO;
    infoMsgLbl.text=[NSString stringWithString:msg];
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationDelay:0];
    [UIView setAnimationDuration:0.5];
    [UIView setAnimationDelegate:self];
    frame=infoMsgView.frame;
    frame.origin.y=self.view.frame.size.height-64-32;
    infoMsgView.frame=frame;
    [UIView setAnimationDidStopSelector:@selector(closePopup)];
    [UIView commitAnimations];
}
-(void) closePopup {
    CGRect frame;
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationDelay:1.0];
    [UIView setAnimationDuration:0.5];
    [UIView setAnimationDelegate:self];
    frame=infoMsgView.frame;
    frame.origin.y=self.view.frame.size.height;
    infoMsgView.frame=frame;
    [UIView setAnimationDidStopSelector:@selector(hidePopup)];
    [UIView commitAnimations];
}


#pragma mark -
#pragma mark Memory management

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Relinquish ownership any cached data, images, etc. that aren't in use.
}
- (void)viewDidUnload {
    // Relinquish ownership of anything that can be recreated in viewDidLoad or on demand.
    // For example: self.myOutlet = nil;;
}
- (void)dealloc {
    [waitingView removeFromSuperview];
    waitingView=nil;

    [waitingViewPlayer removeFromSuperview];
    waitingViewPlayer=nil;

    [self.searchDebounceTimer invalidate];
    self.searchDebounceTimer = nil;

    if (_htmlWebView) {
        [_htmlWebView removeFromSuperview];
        _htmlWebView = nil;
    }

    if (mSearchText) {
        mSearchText=nil;
    }
    if (dbWEB_nb_entries) {
        for (int i=0;i<dbWEB_nb_entries;i++) {
            dbWEB_entries_data[i].label=nil;
            dbWEB_entries_data[i].fullpath=nil;
            dbWEB_entries_data[i].URL=nil;
            dbWEB_entries_data[i].info=nil;
            dbWEB_entries_data[i].img_URL=nil;
        }
        free(dbWEB_entries_data);
    }
    if (search_dbWEB_nb_entries) {
        for (int j=0;j<search_dbWEB_entries_count;j++) {
            search_dbWEB_entries[j].label=nil;
            search_dbWEB_entries[j].fullpath=nil;
            search_dbWEB_entries[j].URL=nil;
            search_dbWEB_entries[j].info=nil;
            search_dbWEB_entries[j].img_URL=nil;
        }
        search_dbWEB_entries=NULL;
        search_dbWEB_nb_entries=0;
        free(search_dbWEB_entries_data);
    }
    if (mFileMngr) {
        mFileMngr=nil;
    }
    
    if (self.mdzChangeObserverToken) {
        [[NSNotificationCenter defaultCenter] removeObserver:self.mdzChangeObserverToken];
        self.mdzChangeObserverToken = nil;
    }
    //[super dealloc];
}

#pragma mark -
#pragma mark - UINavigationControllerDelegate

- (id <UIViewControllerAnimatedTransitioning>)navigationController:(UINavigationController *)navigationController
                                   animationControllerForOperation:(UINavigationControllerOperation)operation
                                                fromViewController:(UIViewController *)fromVC
                                                  toViewController:(UIViewController *)toVC
{
    return [[TTFadeAnimator alloc] init];
}

#pragma mark - LoadingView related stuff

- (void) cancelPushed {
    detailViewController.mplayer.extractPendingCancel=true;
    [detailViewController setCancelStatus:true];
    [detailViewController hideWaitingCancel];
    [detailViewController hideWaitingProgress];
    [detailViewController updateWaitingDetail:NSLocalizedString(@"Cancelling...",@"")];
        
    [self hideWaitingCancel];
    [self hideWaitingProgress];
    [self updateWaitingDetail:NSLocalizedString(@"Cancelling...",@"")];
}

-(void) updateLoadingInfos: (NSTimer *) theTimer {
    [waitingViewPlayer.progressView setProgress:detailViewController.waitingView.progressView.progress animated:YES];
}


- (void) observeValueForKeyPath:(NSString *)keyPath
                       ofObject:(id)object
                         change:(NSDictionary<NSKeyValueChangeKey,id> *)change
                        context:(void *)context
{
    if (context==LoadingProgressObserverContext){
        WaitingView *wv = object;
        if (wv.hidden==false) {
            [waitingViewPlayer resetCancelStatus];
            waitingViewPlayer.hidden=detailViewController.waitingView.hidden;
            waitingViewPlayer.btnStopCurrentAction.hidden=detailViewController.waitingView.btnStopCurrentAction.hidden;
            waitingViewPlayer.progressView.progress=detailViewController.waitingView.progressView.progress;
            waitingViewPlayer.progressView.hidden=detailViewController.waitingView.progressView.hidden;
            waitingViewPlayer.lblTitle.text=[NSString stringWithString:detailViewController.waitingView.lblTitle.text];
            waitingViewPlayer.lblDetail.text=[NSString stringWithString:detailViewController.waitingView.lblDetail.text];
        }
        waitingViewPlayer.hidden=wv.hidden;
        
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}


@end
