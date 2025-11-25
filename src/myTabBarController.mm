//
//  myTabBarController.m
//  modizer4
//
//  Created by Yohann Magnien on 14/06/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//


#import "myTabBarController.h"
#import "TTFadeAnimator.h"

#import "ModizerConstants.h"
#import "ModizerTypes.h"

#import "CarPlayAndRemoteManagement.h"
#import "WelcomeVC.h"
#import "SceneDelegate.h"
#import "StoreManager.h"


extern int shiftPressedL,shiftPressedR;
extern int move_cursorL,move_cursorR,keyDel;

@implementation myTabBarController

@synthesize welcomePages;
@synthesize detailViewControllerIphone;
@synthesize playlistVC;
@synthesize rootViewControllerIphone;
@synthesize onlineVC;
@synthesize searchVC;
@synthesize moreVC;
@synthesize webBrowser;
@synthesize downloadVC;
@synthesize aboutVC;

@synthesize animatedLaunchVC;
@synthesize cpMngt;


- (UIViewController *)visibleViewController:(UIViewController *)rootViewController
{
    if ([rootViewController isKindOfClass:[UITabBarController class]])
    {
        UIViewController *selectedViewController = ((UITabBarController *)rootViewController).selectedViewController;

        return [self visibleViewController:selectedViewController];
    }
    if ([rootViewController isKindOfClass:[UINavigationController class]])
    {
        UIViewController *lastViewController = [[((UINavigationController *)rootViewController) viewControllers] lastObject];

        return [self visibleViewController:lastViewController];
    }
    
    if (rootViewController.presentedViewController == nil)
    {
        return rootViewController;
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UINavigationController class]])
    {
        UINavigationController *navigationController = (UINavigationController *)rootViewController.presentedViewController;
        UIViewController *lastViewController = [[navigationController viewControllers] lastObject];

        return [self visibleViewController:lastViewController];
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UITabBarController class]])
    {
        UITabBarController *tabBarController = (UITabBarController *)rootViewController.presentedViewController;
        UIViewController *selectedViewController = tabBarController.selectedViewController;

        return [self visibleViewController:selectedViewController];
    }

    UIViewController *presentedViewController = (UIViewController *)rootViewController.presentedViewController;

    return [self visibleViewController:presentedViewController];
}


- (UIStatusBarStyle)preferredStatusBarStyle {    
    return UIStatusBarStyleDefault;
}

- (UIViewController *)childViewControllerForStatusBarStyle {
    UIViewController *vc=[self visibleViewController:self];
    return vc;
}

- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
    //    return UIInterfaceOrientationMaskAllButUpsideDown;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    self.navigationController.delegate = self;
    
    [self setNeedsStatusBarAppearanceUpdate];
    
    [self showAnimatedLaunchOverlay];
}

- (id)findChildOfClass:(Class)cls {
    for (UIViewController *vc in self.viewControllers) {
        // If embedded in a nav controller, check its root
        if ([vc isKindOfClass:[UINavigationController class]]) {
            UIViewController *root = ((UINavigationController *)vc).viewControllers.firstObject;
            if ([root isKindOfClass:cls]) {
                return root;
            }
        } else if ([vc isKindOfClass:cls]) {
            return vc;
        }
    }
    return nil;
}

- (SceneDelegate *)currentSceneDelegate {
    UIWindowScene *windowScene = nil;
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]]) {
            UIWindowScene *ws = (UIWindowScene *)scene;
                windowScene = ws;
                break;
        }
    }
    return (SceneDelegate *)windowScene.delegate;
}

- (void)registerVCinAppDelegate {
    AppDelegate_Phone *appDelegate = (AppDelegate_Phone *)[[UIApplication sharedApplication] delegate];
    appDelegate.detailViewControlleriPhone = detailViewControllerIphone;
    appDelegate.rootViewControlleriPhone = rootViewControllerIphone;
    appDelegate.tabBarC = self;
    appDelegate.playlistVC = playlistVC;
    appDelegate.downloadVC = downloadVC;
        
    SceneDelegate *sceneDelegate = [self currentSceneDelegate];
    sceneDelegate.detailViewControlleriPhone = detailViewControllerIphone;
    sceneDelegate.rootViewControlleriPhone = rootViewControllerIphone;
    sceneDelegate.tabBarController = self;
    sceneDelegate.playlistVC = playlistVC;
    sceneDelegate.downloadVC =downloadVC;
}

- (void)goToNextWelcomePage {
    if (welcomePageIndex >= [self.welcomePages count] - 1) {
        // Already on last page
        return;
    }
    welcomePageIndex++;
    [myPVC setViewControllers:@[self.welcomePages[welcomePageIndex]]
                    direction:UIPageViewControllerNavigationDirectionForward
                     animated:YES
                   completion:nil];
}

- (void)goToPreviousWelcomePage {
    if (welcomePageIndex <= 0) {
        // Already on first page
        return;
    }
    welcomePageIndex--;
    [myPVC setViewControllers:@[self.welcomePages[welcomePageIndex]]
                    direction:UIPageViewControllerNavigationDirectionReverse
                     animated:YES
                   completion:nil];
}

- (void)goToWelcomePageAtIndex:(NSInteger)index {
    if (index < 0 || index >= [self.welcomePages count]) {
        return;
    }
    UIPageViewControllerNavigationDirection direction =
        (index > welcomePageIndex) ? UIPageViewControllerNavigationDirectionForward
                                   : UIPageViewControllerNavigationDirectionReverse;
    welcomePageIndex = index;
    [myPVC setViewControllers:@[self.welcomePages[index]]
                    direction:direction
                     animated:YES
                   completion:nil];
}

- (UIImage *)createScanlinePattern:(CGSize)size {
    // Create scanline pattern
    UIGraphicsBeginImageContextWithOptions(size, NO, 0.0);
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    // Draw scanlines
    CGFloat scanlineHeight = 1.0; // Height of each scanline
    CGFloat scanlineSpacing = 2.0; // Distance between scanlines
    
    for (CGFloat y = 0; y < size.height; y += scanlineSpacing) {
        CGContextSetFillColorWithColor(context, [UIColor colorWithWhite:0.0 alpha:0.45].CGColor);
        CGContextFillRect(context, CGRectMake(0, y, size.width, scanlineHeight));
    }
    
    UIImage *scanlineImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    
    return scanlineImage;
}

- (void)applyGradientToLabel:(UILabel *)label {
    // Remove any existing gradient layers
    for (CALayer *layer in label.layer.sublayers.copy) {
        if ([layer isKindOfClass:[CAGradientLayer class]]) {
            [layer removeFromSuperlayer];
        }
    }
    
    // Create gradient layer
    CAGradientLayer *gradientLayer = [CAGradientLayer layer];
    gradientLayer.frame = label.bounds;

#define COL1 0xFF4FB3
#define COL2 0xC44CFF
#define COL3 0x4AA8FF
    
#define RED(x) (((x>>16)&0xFF)/255.0)
#define GREEN(x) (((x>>8)&0xFF)/255.0)
#define BLUE(x) (((x>>0)&0xFF)/255.0)
    // Define gradient colors (adjust these to your preference)
    gradientLayer.colors = @[
        (id)[UIColor colorWithRed:RED(COL1) green:GREEN(COL1) blue:BLUE(COL1) alpha:1.0].CGColor,
        (id)[UIColor colorWithRed:RED(COL2) green:GREEN(COL2) blue:BLUE(COL2) alpha:1.0].CGColor,
        (id)[UIColor colorWithRed:RED(COL3) green:GREEN(COL3) blue:BLUE(COL3) alpha:1.0].CGColor,
    ];

    
    // Set gradient direction (0,0 to 1,0 = left to right, 0,0 to 0,1 = top to bottom)
    gradientLayer.startPoint = CGPointMake(0.0, 0.0);
    gradientLayer.endPoint = CGPointMake(1.0, 1.0);
    
    // Render the gradient into an image
    UIGraphicsBeginImageContextWithOptions(label.bounds.size, NO, 0.0);
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    // Draw gradient
    [gradientLayer renderInContext:context];
    
    // Overlay scanlines
    UIImage *scanlineImage = [self createScanlinePattern:label.bounds.size];
    [scanlineImage drawInRect:CGRectMake(0, 0, label.bounds.size.width, label.bounds.size.height) blendMode:kCGBlendModeMultiply alpha:1.0];
    
    UIImage *finalImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    
    // Apply gradient + scanlines as text color
    label.textColor = [UIColor colorWithPatternImage:finalImage];
}

- (void) setupWelcomePages {
    welcomePage1=[[WelcomeVC alloc] initWithNibName:@"WelcomeView_1Image" bundle:[NSBundle mainBundle]];
    welcomePage2=[[WelcomeVC alloc] initWithNibName:@"WelcomeView_2Images" bundle:[NSBundle mainBundle]];
    welcomePage3=[[WelcomeVC alloc] initWithNibName:@"WelcomeView_4Images" bundle:[NSBundle mainBundle]];
    welcomePage4=[[WelcomeVC alloc] initWithNibName:@"WelcomeView_1Image" bundle:[NSBundle mainBundle]];
    
    [welcomePage1 loadViewIfNeeded];
    [welcomePage2 loadViewIfNeeded];
    [welcomePage3 loadViewIfNeeded];
    [welcomePage4 loadViewIfNeeded];
    
    //Page 1
    welcomePage1.topLabel.text=NSLocalizedString(
@"Welcome to Modizer!\n",@"");
    welcomePage1.topLabel.font = [UIFont fontWithName:@"Orbitron-Regular" size:24];
    welcomePage1.imageView1.image = [UIImage imageNamed:@"welcome_localBrowser.png"];
    welcomePage1.leftBtn.hidden=true;
    welcomePage1.rightBtn.hidden=false;
    [welcomePage1.rightBtn addTarget:self action:@selector(goToNextWelcomePage) forControlEvents:UIControlEventTouchUpInside];
    [welcomePage1.exitBtn setTitle:NSLocalizedString(@"Skip",@"") forState:UIControlStateNormal];
    welcomePage1.messageLabel.text=NSLocalizedString(@""
"Your gateway to retro and tracker music.\n"
"Power up your device with legendary game tunes, iconic tracker modules,and timeless chiptune classics.",@"");
    welcomePage1.messageLabel.font = [UIFont fontWithName:@"Montserrat-Regular" size:16];
    
    //Page 2
    welcomePage2.topLabel.text=NSLocalizedString(@"Level up your library",@"");
    welcomePage2.topLabel.font = [UIFont fontWithName:@"Orbitron-Regular" size:24];
    welcomePage2.imageView1.image = [UIImage imageNamed:@"welcome_online.png"];
    welcomePage2.imageView2.image = [UIImage imageNamed:@"welcome_playlist.png"];
    welcomePage2.leftBtn.hidden=false;
    welcomePage2.rightBtn.hidden=false;
    [welcomePage2.leftBtn addTarget:self action:@selector(goToPreviousWelcomePage) forControlEvents:UIControlEventTouchUpInside];
    [welcomePage2.rightBtn addTarget:self action:@selector(goToNextWelcomePage) forControlEvents:UIControlEventTouchUpInside];
    [welcomePage2.exitBtn setTitle:NSLocalizedString(@"Skip",@"") forState:UIControlStateNormal];
    welcomePage2.messageLabel.text=NSLocalizedString(@""
"Browse and stream from online catalogs, to complete your own collections.\nBuild, edit, and listen to playlists effortlessly.",@"");
    welcomePage2.messageLabel.font = [UIFont fontWithName:@"Montserrat-Regular" size:16];
    
    //Page 3
    welcomePage3.topLabel.text=NSLocalizedString(@"Sound meets visuals.",@"");
    welcomePage3.topLabel.font = [UIFont fontWithName:@"Orbitron-Regular" size:24];
    welcomePage3.imageView1.image = [UIImage imageNamed:@"welcome_playerView1.png"];
    welcomePage3.imageView2.image = [UIImage imageNamed:@"welcome_playerView2.png"];
    welcomePage3.imageView3.image = [UIImage imageNamed:@"welcome_playerView3.png"];
    welcomePage3.imageView4.image = [UIImage imageNamed:@"welcome_playerView4.png"];
    welcomePage3.leftBtn.hidden=false;
    welcomePage3.rightBtn.hidden=false;
    [welcomePage3.leftBtn addTarget:self action:@selector(goToPreviousWelcomePage) forControlEvents:UIControlEventTouchUpInside];
    [welcomePage3.rightBtn addTarget:self action:@selector(goToNextWelcomePage) forControlEvents:UIControlEventTouchUpInside];
    [welcomePage3.exitBtn setTitle:NSLocalizedString(@"Skip",@"") forState:UIControlStateNormal];
    welcomePage3.messageLabel.text=NSLocalizedString(@""
"Unlock classic oscilloscope looks, spectrum bars, piano rolls, trackers view and modern FX based on ProjectM/Milkdrop.\nLet Modizer paint each track with motion and color.",@"");
    welcomePage3.messageLabel.font = [UIFont fontWithName:@"Montserrat-Regular" size:16];
    
    //Page 4
    welcomePage4.topLabel.text=NSLocalizedString(@"Made with passion,\noffered for free.",@"");
    welcomePage4.topLabel.font = [UIFont fontWithName:@"Orbitron-Regular" size:24];
    welcomePage4.imageView1.image = [UIImage imageNamed:@"welcome_more.png"];
    welcomePage4.leftBtn.hidden=false;
    welcomePage4.rightBtn.hidden=true;
    [welcomePage4.leftBtn addTarget:self action:@selector(goToPreviousWelcomePage) forControlEvents:UIControlEventTouchUpInside];
    [welcomePage4.exitBtn setTitle:NSLocalizedString(@"Close",@"") forState:UIControlStateNormal];
    welcomePage4.messageLabel.text=NSLocalizedString(@""
"If you enjoy the app, tips are a great way to support its ongoing development.\nThank you for helping keep Modizer alive and evolving.",@"");
    welcomePage4.messageLabel.font = [UIFont fontWithName:@"Montserrat-Regular" size:16];
    
    [welcomePage1.exitBtn addTarget:self action:@selector(exitWelcomePages) forControlEvents:UIControlEventTouchUpInside];
    [welcomePage2.exitBtn addTarget:self action:@selector(exitWelcomePages) forControlEvents:UIControlEventTouchUpInside];
    [welcomePage3.exitBtn addTarget:self action:@selector(exitWelcomePages) forControlEvents:UIControlEventTouchUpInside];
    [welcomePage4.exitBtn addTarget:self action:@selector(exitWelcomePages) forControlEvents:UIControlEventTouchUpInside];
    
    self.welcomePages= @[welcomePage1,welcomePage2,welcomePage3,welcomePage4];
    
    myPVC=[[UIPageViewController alloc] initWithTransitionStyle:UIPageViewControllerTransitionStyleScroll navigationOrientation:UIPageViewControllerNavigationOrientationHorizontal options:NULL];
    welcomePageIndex=0;
    [myPVC setViewControllers:@[welcomePages[welcomePageIndex]] direction:UIPageViewControllerNavigationDirectionForward animated:YES completion:nil];
    
    myPVC.dataSource=self;
    myPVC.delegate=self;
    
    // Apply gradients after layout
    dispatch_async(dispatch_get_main_queue(), ^{
        [self applyGradientToLabel:self->welcomePage1.topLabel];
        [self applyGradientToLabel:self->welcomePage2.topLabel];
        [self applyGradientToLabel:self->welcomePage3.topLabel];
        [self applyGradientToLabel:self->welcomePage4.topLabel];
    });
}

- (void)enablePageControlTaps {
    for (UIView *view in myPVC.view.subviews) {
        if ([view isKindOfClass:[UIPageControl class]]) {
            UIPageControl *pageControl = (UIPageControl *)view;
            pageControl.userInteractionEnabled = YES;
            [pageControl addTarget:self action:@selector(pageControlTapped:) forControlEvents:UIControlEventValueChanged];
            break;
        }
    }
}

- (void)pageControlTapped:(UIPageControl *)pageControl {
    NSInteger targetPage = pageControl.currentPage;
    [self goToWelcomePageAtIndex:targetPage];
}



- (void)viewDidLoad {
    START_PROFILE
	[super viewDidLoad];
    self.navigationController.delegate = self;
    
    //self.view.backgroundColor = [UIColor clearColor];
    
    // iOS 15+ Fix: Configure appearance for tab bar and navigation bar
    if (@available(iOS 15.0, *)) {
        // Configure Tab Bar Appearance
        UITabBarAppearance *tabBarAppearance = [[UITabBarAppearance alloc] init];
        [tabBarAppearance configureWithDefaultBackground];
        
        // You can customize colors here if needed:
        // tabBarAppearance.backgroundColor = [UIColor systemBackgroundColor];
        
        self.tabBar.standardAppearance = tabBarAppearance;
        self.tabBar.scrollEdgeAppearance = tabBarAppearance;
        
        // Configure Navigation Bar Appearance for all child navigation controllers
        UINavigationBarAppearance *navBarAppearance = [[UINavigationBarAppearance alloc] init];
        [navBarAppearance configureWithDefaultBackground];
        
        // You can customize colors here if needed:
        // navBarAppearance.backgroundColor = [UIColor systemBackgroundColor];
        
        // Apply to all navigation controllers in tabs
        for (UIViewController *vc in self.viewControllers) {
            if ([vc isKindOfClass:[UINavigationController class]]) {
                UINavigationController *nav = (UINavigationController *)vc;
                nav.navigationBar.standardAppearance = navBarAppearance;
                nav.navigationBar.scrollEdgeAppearance = navBarAppearance;
                nav.navigationBar.compactAppearance = navBarAppearance;
            }
        }
    }
    
    if (@available(iOS 18.0, *)) {
        if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad) {
            self.traitOverrides.horizontalSizeClass = UIUserInterfaceSizeClassCompact;//UIUserInterfaceSizeClassRegular;
        }
    }
    
    // Resolve detailViewControllerIphone
    self.rootViewControllerIphone = [self findChildOfClass:[RootViewControllerLocalBrowser class]];
    self.playlistVC = [self findChildOfClass:[RootViewControllerPlaylist class]];
    self.onlineVC = [self findChildOfClass:[OnlineViewController class]];
    self.searchVC = [self findChildOfClass:[SearchViewController class]];
    self.moreVC = [self findChildOfClass:[MoreViewController class]];
    
    self.detailViewControllerIphone = [self findChildOfClass:[DetailViewControllerIphone class]];
    self.webBrowser = [self findChildOfClass:[WebBrowser class]];
    self.downloadVC = [self findChildOfClass:[DownloadViewController class]];
    self.aboutVC = [self findChildOfClass:[AboutViewController class]];
    
    [self.rootViewControllerIphone loadViewIfNeeded];
    [self.playlistVC loadViewIfNeeded];
    [self.onlineVC loadViewIfNeeded];
    [self.searchVC loadViewIfNeeded];
    [self.moreVC loadViewIfNeeded];
    
    [self.detailViewControllerIphone loadViewIfNeeded];
    [self.webBrowser loadViewIfNeeded];
    [self.downloadVC loadViewIfNeeded];
    [self.aboutVC loadViewIfNeeded];
    
    // Build a filtered list of tab view controllers by class
    NSArray<Class> *excludedClasses = @[
        // List classes to exclude here, e.g.:
        [DetailViewControllerIphone class],
        [AboutViewController class],
        [DownloadViewController class],
        [WebBrowser class],
        // [MoreViewController class]
    ];
    
    //Initiate storeManager
    [StoreManager sharedManager];
    
    //check if new version
    if (detailViewControllerIphone.not_expected_version) {
        //show Welcome Screen
        settings[GLOB_ShowWelcome].detail.mdz_boolswitch.switch_value=1;
    }
    
    self.downloadVC.barItem=moreVC.navigationController.tabBarItem;
    [self.downloadVC refreshDownloadCountBadge];
    
    NSMutableArray<UIViewController *> *filteredTabs = [NSMutableArray array];
    for (UIViewController *vc in self.viewControllers) {
        BOOL shouldExclude = NO;
        Class candidateClass = [vc class];
        if ([vc isKindOfClass:[UINavigationController class]]) {
            UINavigationController *nav = (UINavigationController *)vc;
            UIViewController *root = nav.viewControllers.firstObject;
            if (root != nil) {
                candidateClass = [root class];
            }
        }
        for (Class cls in excludedClasses) {
            if ([candidateClass isSubclassOfClass:cls]) {
                shouldExclude = YES;
                break;
            }
        }
        if (!shouldExclude) {
            [filteredTabs addObject:vc];
        }
    }
    [self setViewControllers:filteredTabs animated:NO];
    
    // iOS 15+ Fix: Reapply navigation bar appearance after setting view controllers
    if (@available(iOS 15.0, *)) {
        UINavigationBarAppearance *navBarAppearance = [[UINavigationBarAppearance alloc] init];
        [navBarAppearance configureWithDefaultBackground];
        
        for (UIViewController *vc in filteredTabs) {
            if ([vc isKindOfClass:[UINavigationController class]]) {
                UINavigationController *nav = (UINavigationController *)vc;
                nav.navigationBar.standardAppearance = navBarAppearance;
                nav.navigationBar.scrollEdgeAppearance = navBarAppearance;
                nav.navigationBar.compactAppearance = navBarAppearance;
            }
        }
    }
    
    // Perform initial setup that previously lived in AppDelegate
    [self.rootViewControllerIphone createEditableCopyOfDatabaseIfNeeded:FALSE quiet:0];

    // Initialize CarPlay management
    self.cpMngt = [[CarPlayAndRemoteManagement alloc] init];
    self.cpMngt.detailViewController = detailViewControllerIphone;
    self.cpMngt.rootVCLocalB = rootViewControllerIphone;
    [self.cpMngt initCarPlayAndRemote];
    
    
    //Register various key VC in App Delegate
    [self registerVCinAppDelegate];

    
    // Configure notifications delegate to detail controller
    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    center.delegate = detailViewControllerIphone;
    [center requestAuthorizationWithOptions:UNAuthorizationOptionAlert completionHandler:^(BOOL granted, NSError * _Nullable error) {
        if (!granted) mdzNotificationAllowed=false;
        else mdzNotificationAllowed=true;
    }];
    
    
    UIWindowScene *windowScene;
    for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]]) {
            windowScene = (UIWindowScene*) scene;
            break;
        }
    }
    
    [self setupWelcomePages];
    
    //Faster loading for debug
#ifdef DEBUG_MODIZER
    //[window addSubview:[animatedLaunchVC view]];
#else
//    [window addSubview:[animatedLaunchVC view]];
#endif
    
    END_PROFILE
}

- (void)presentWelcomePages {
    if (!settings[GLOB_ShowWelcome].detail.mdz_boolswitch.switch_value) return;
    
    // Don't present if already presenting or presented
    if (myPVC.presentingViewController != nil || myPVC.isBeingPresented) {
        return;
    }
    
    if (myPVC) {
        [self presentViewController:myPVC animated:NO completion:^{
            // Enable page control taps after presentation
            [self enablePageControlTaps];
        }];
        //only show it once
        settings[GLOB_ShowWelcome].detail.mdz_boolswitch.switch_value=0;
    }
}

- (void)exitWelcomePages {
    [myPVC dismissViewControllerAnimated:true completion:^{
    }];
}

- (nullable UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerBeforeViewController:(UIViewController *)viewController {
    WelcomeVC *last_item=NULL;
    for (WelcomeVC *item in self.welcomePages) {
        if (item==viewController) {
            break;
        }
        last_item=item;
    }
    return last_item;
}
- (nullable UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerAfterViewController:(UIViewController *)viewController {
    WelcomeVC *last_item=NULL;
    WelcomeVC *next_item=NULL;
    for (WelcomeVC *item in self.welcomePages) {
        if (last_item==viewController) {
            next_item=item;
            break;
        }
        last_item=item;
    }
    return next_item;
}

- (NSInteger)presentationCountForPageViewController:(UIPageViewController *)pageViewController {
    // The number of items reflected in the page indicator.
    return 4;
}

- (NSInteger)presentationIndexForPageViewController:(UIPageViewController *)pageViewController {
    // The selected item reflected in the page indicator.
    return welcomePageIndex;
}

- (void)pageViewController:(UIPageViewController *)pageViewController 
        didFinishAnimating:(BOOL)finished 
   previousViewControllers:(NSArray<UIViewController *> *)previousViewControllers 
       transitionCompleted:(BOOL)completed {
    
    if (completed) {
        UIViewController *currentVC = pageViewController.viewControllers.firstObject;
        welcomePageIndex = [self.welcomePages indexOfObject:currentVC];
    }
}

- (void)showAnimatedLaunchOverlay {
    if (self.animatedLaunchVC != nil) { return; }

    AnimatedLaunchVC *vc = [[AnimatedLaunchVC alloc] initWithNibName:@"AnimatedLaunch" bundle:[NSBundle mainBundle]];
    vc.localBrowserVC = self.rootViewControllerIphone;
    vc.tabVC = self;
    
    // Load the view first
    [vc loadViewIfNeeded];
    
    // Forward appearance to child
    [vc beginAppearanceTransition:YES animated:NO];

    // Try adding to window for guaranteed top-level display
    UIWindow *window = self.view.window;
    if (window) {
        vc.view.frame = window.bounds;
        vc.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        vc.view.alpha = 1.0;
        
        [window addSubview:vc.view];
    } else {
        // Fallback to self.view if window not available yet
        vc.view.frame = self.view.bounds;
        vc.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        vc.view.alpha = 1.0;
        [self.view addSubview:vc.view];
        [self.view bringSubviewToFront:vc.view];
    }

    // Finish forwarding
    [vc endAppearanceTransition];

    [vc didMoveToParentViewController:self];

    self.animatedLaunchVC = vc;
}
//- (void)hideAnimatedLaunchOverlay {
//    if (!self.animatedLaunchVC) { return; }
//
//    [self.animatedLaunchVC willMoveToParentViewController:nil];
//
//    // Forward disappearance to child
//    [self.animatedLaunchVC beginAppearanceTransition:NO animated:YES];
//
//    [UIView animateWithDuration:0.3 animations:^{
//        self.animatedLaunchVC.view.alpha = 0.0;
//    } completion:^(BOOL finished) {
//        [self.animatedLaunchVC.view removeFromSuperview];
//        
//        // Finish forwarding
//        [self.animatedLaunchVC endAppearanceTransition];
//        
//        [self.animatedLaunchVC removeFromParentViewController];
//        self.animatedLaunchVC = nil;
//    }];
//}
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
    static bool firstcall=true;
    
    if (firstcall) {
        firstcall=false;
        
        // Force layout of all visible views first
        [self.view layoutIfNeeded];
        
        // Let the table views fully load their data
        if ([self.selectedViewController isKindOfClass:[UINavigationController class]]) {
            UINavigationController *nav = (UINavigationController *)self.selectedViewController;
            [nav.topViewController.view layoutIfNeeded];
        } else {
            [self.selectedViewController.view layoutIfNeeded];
        }
        
        //[self showAnimatedLaunchOverlay];
    }
    
    
}

-(void) openURL:(NSURL *)url {
    if ([url isFileURL]) {
        NSString *filepath;
        filepath=[url path];
        
        NSString *imported_filepath;
        NSError *err;
        NSFileManager *mFileMngr=[[NSFileManager alloc] init];
        
        [mFileMngr createDirectoryAtPath:[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Downloads"] withIntermediateDirectories:true attributes:NULL error:NULL];
        
        imported_filepath=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Downloads"],[filepath lastPathComponent]];
        //////////////////
        ///Get access
        if ([url startAccessingSecurityScopedResource]) {
            ////////////////////
            //Download from icould if required
            
            NSNumber *isDownloadedValue = NULL;
            if ([mFileMngr isUbiquitousItemAtURL:url]) {
                BOOL success = [url getResourceValue:&isDownloadedValue forKey:NSURLUbiquitousItemIsDownloadedKey error:NULL];
                if (success && ![isDownloadedValue boolValue]) {
                    [[NSFileManager defaultManager] startDownloadingUbiquitousItemAtURL:url error:NULL];
                    
                    
                    //                    UIAlertView *alertDownloading = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"File is not available locally.\nTrigerring download from iCloud, please check in 'Files' application.",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil];
                    //                    if (alertDownloading) [alertDownloading show];
                    UIAlertController *alertDownloading = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Warning",@"")
                                                                                              message:NSLocalizedString(@"File is not available locally.\nTrigerring download from iCloud, please check in 'Files' application.",@"")
                                                                                       preferredStyle:UIAlertControllerStyleAlert];
                    UIAlertAction* closeAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Close",@"") style:UIAlertActionStyleCancel
                                                                        handler:^(UIAlertAction * action) {
                    }];
                    [alertDownloading addAction:closeAction];
                    [self presentViewController:alertDownloading animated:YES completion:nil];
                    
                    //return YES;
                }
            }
            
            if ([mFileMngr copyItemAtPath:filepath toPath:imported_filepath error:&err]) {
                [rootViewControllerIphone refreshViewAfterDownload];
            } else {
            }
            [url stopAccessingSecurityScopedResource];
        } else  {
        }
        
        NSString *shortfilepath=imported_filepath=[NSString stringWithFormat:@"Documents/Downloads/%@",[filepath lastPathComponent]];

        t_playlist *pl;
        pl=(t_playlist*)calloc(1,sizeof(t_playlist));
        
        pl->nb_entries=1;
        pl->entries[0].label=[shortfilepath lastPathComponent];
        pl->entries[0].fullpath=shortfilepath;
        pl->entries[0].ratings=-1;
        pl->entries[0].playcounts=0;
        [detailViewControllerIphone play_listmodules:pl start_index:0];
        free(pl);
    }
}

#pragma mark - Key Commands

- (NSArray *)keyCommands
{
    return @[ [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow  modifierFlags:0 action:@selector(leftPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow   modifierFlags:0 action:@selector(rightPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow   modifierFlags:UIKeyModifierAlternate action:@selector(leftAltPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow   modifierFlags:UIKeyModifierAlternate action:@selector(rightAltPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow   modifierFlags:UIKeyModifierCommand action:@selector(leftCmdPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow   modifierFlags:UIKeyModifierCommand action:@selector(rightCmdPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputUpArrow   modifierFlags:0 action:@selector(upPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputDownArrow   modifierFlags:0 action:@selector(downPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"1"   modifierFlags:0 action:@selector(key1Pressed)],
              [UIKeyCommand keyCommandWithInput:@"&"   modifierFlags:0 action:@selector(key1Pressed)],
              [UIKeyCommand keyCommandWithInput:@"1"   modifierFlags:UIKeyModifierAlternate action:@selector(key1AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"&"   modifierFlags:UIKeyModifierAlternate action:@selector(key1AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"2"   modifierFlags:0 action:@selector(key2Pressed)],
              [UIKeyCommand keyCommandWithInput:@"é"   modifierFlags:0 action:@selector(key2Pressed)],
              [UIKeyCommand keyCommandWithInput:@"2"   modifierFlags:UIKeyModifierAlternate action:@selector(key2AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"é"   modifierFlags:UIKeyModifierAlternate action:@selector(key2AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"3"   modifierFlags:0 action:@selector(key3Pressed)],
              [UIKeyCommand keyCommandWithInput:@"\""   modifierFlags:0 action:@selector(key3Pressed)],
              [UIKeyCommand keyCommandWithInput:@"3"   modifierFlags:UIKeyModifierAlternate action:@selector(key3AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"\""   modifierFlags:UIKeyModifierAlternate action:@selector(key3AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"4"   modifierFlags:0 action:@selector(key4Pressed)],
              [UIKeyCommand keyCommandWithInput:@"'"   modifierFlags:0 action:@selector(key4Pressed)],
              [UIKeyCommand keyCommandWithInput:@"4"   modifierFlags:UIKeyModifierAlternate action:@selector(key4AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"'"   modifierFlags:UIKeyModifierAlternate action:@selector(key4AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"5"   modifierFlags:0 action:@selector(key5Pressed)],
              [UIKeyCommand keyCommandWithInput:@"("   modifierFlags:0 action:@selector(key5Pressed)],
              [UIKeyCommand keyCommandWithInput:@"5"   modifierFlags:UIKeyModifierAlternate action:@selector(key5AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"("   modifierFlags:UIKeyModifierAlternate action:@selector(key5AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"6"   modifierFlags:0 action:@selector(key6Pressed)],
              [UIKeyCommand keyCommandWithInput:@"§"   modifierFlags:0 action:@selector(key6Pressed)],
              [UIKeyCommand keyCommandWithInput:@"6"   modifierFlags:UIKeyModifierAlternate action:@selector(key6AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"§"   modifierFlags:UIKeyModifierAlternate action:@selector(key6AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"7"   modifierFlags:0 action:@selector(key7Pressed)],
              [UIKeyCommand keyCommandWithInput:@"è"   modifierFlags:0 action:@selector(key7Pressed)],
              [UIKeyCommand keyCommandWithInput:@"7"   modifierFlags:UIKeyModifierAlternate action:@selector(key7AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"è"   modifierFlags:UIKeyModifierAlternate action:@selector(key7AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"8"   modifierFlags:0 action:@selector(key8Pressed)],
              [UIKeyCommand keyCommandWithInput:@"!"   modifierFlags:0 action:@selector(key8Pressed)],
              [UIKeyCommand keyCommandWithInput:@"8"   modifierFlags:UIKeyModifierAlternate action:@selector(key8AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"!"   modifierFlags:UIKeyModifierAlternate action:@selector(key8AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"9"   modifierFlags:0 action:@selector(key9Pressed)],
              [UIKeyCommand keyCommandWithInput:@"ç"   modifierFlags:0 action:@selector(key9Pressed)],
              [UIKeyCommand keyCommandWithInput:@"9"   modifierFlags:UIKeyModifierAlternate action:@selector(key9AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"ç"   modifierFlags:UIKeyModifierAlternate action:@selector(key9AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"0"   modifierFlags:0 action:@selector(key0Pressed)],
              [UIKeyCommand keyCommandWithInput:@"à"   modifierFlags:0 action:@selector(key0Pressed)],
              [UIKeyCommand keyCommandWithInput:@"0"   modifierFlags:UIKeyModifierAlternate action:@selector(key0AltPressed)],
              [UIKeyCommand keyCommandWithInput:@"à"   modifierFlags:UIKeyModifierAlternate action:@selector(key0AltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"i"   modifierFlags:0 action:@selector(keyIPressed)],
              [UIKeyCommand keyCommandWithInput:@"i"   modifierFlags:UIKeyModifierAlternate action:@selector(keyIAltPressed)],
              [UIKeyCommand keyCommandWithInput:@"e"   modifierFlags:0 action:@selector(keyEPressed)],
              [UIKeyCommand keyCommandWithInput:@"f"   modifierFlags:0 action:@selector(keyFPressed)],
              [UIKeyCommand keyCommandWithInput:@"f"   modifierFlags:UIKeyModifierAlternate action:@selector(keyFAltPressed)],
              [UIKeyCommand keyCommandWithInput:@"s"   modifierFlags:0 action:@selector(keySPressed)],
              [UIKeyCommand keyCommandWithInput:@"s"   modifierFlags:UIKeyModifierAlternate action:@selector(keySAltPressed)],
              [UIKeyCommand keyCommandWithInput:@"h"   modifierFlags:0 action:@selector(keyHPressed)],
              [UIKeyCommand keyCommandWithInput:@"\r"   modifierFlags:0 action:@selector(enterPressed)],
              [UIKeyCommand keyCommandWithInput:@" "   modifierFlags:0 action:@selector(spacePressed)],
              
              [UIKeyCommand keyCommandWithInput:@"b"   modifierFlags:0 action:@selector(keyBPressed)],
              [UIKeyCommand keyCommandWithInput:@"m"   modifierFlags:0 action:@selector(keyMPressed)],
              [UIKeyCommand keyCommandWithInput:@"m"   modifierFlags:UIKeyModifierAlternate action:@selector(keyMAltPressed)],
              [UIKeyCommand keyCommandWithInput:@"o"   modifierFlags:0 action:@selector(keyOPressed)],
              [UIKeyCommand keyCommandWithInput:@"o"   modifierFlags:UIKeyModifierAlternate action:@selector(keyOAltPressed)],
              [UIKeyCommand keyCommandWithInput:@"v"   modifierFlags:0 action:@selector(keyVPressed)],
              [UIKeyCommand keyCommandWithInput:@"t"   modifierFlags:0 action:@selector(keyTPressed)],
              [UIKeyCommand keyCommandWithInput:@"t"   modifierFlags:UIKeyModifierAlternate action:@selector(keyTAltPressed)],
              
              [UIKeyCommand keyCommandWithInput:@"p"   modifierFlags:0 action:@selector(keyPPressed)],
              [UIKeyCommand keyCommandWithInput:@"n"   modifierFlags:0 action:@selector(keyNPressed)],
              [UIKeyCommand keyCommandWithInput:@"l"   modifierFlags:0 action:@selector(keyLPressed)],
              [UIKeyCommand keyCommandWithInput:@"a"   modifierFlags:0 action:@selector(keyAPressed)],
              [UIKeyCommand keyCommandWithInput:@"q"   modifierFlags:0 action:@selector(keyQPressed)],
              
              [UIKeyCommand keyCommandWithInput:UIKeyInputEscape   modifierFlags:0 action:@selector(keyESCPressed)],
              [UIKeyCommand keyCommandWithInput:UIKeyInputDelete   modifierFlags:0 action:@selector(keyDeletePressed)],
              [UIKeyCommand keyCommandWithInput:@"\t"  modifierFlags:0 action:@selector(keyTabPressed)],];
    
}

#pragma mark - Key Action Methods

-(void)key1Pressed {
    [detailViewControllerIphone switchFX:1 change:1];
}
-(void)key1AltPressed {
    [detailViewControllerIphone switchFX:1 change:-1];
}
-(void)key2Pressed {
    [detailViewControllerIphone switchFX:2 change:1];
}
-(void)key2AltPressed {
    [detailViewControllerIphone switchFX:2 change:-1];
}
-(void)key3Pressed {
    [detailViewControllerIphone switchFX:3 change:1];
}
-(void)key3AltPressed {
    [detailViewControllerIphone switchFX:3 change:-1];
}
-(void)key4Pressed {
    [detailViewControllerIphone switchFX:4 change:1];
}
-(void)key4AltPressed {
    [detailViewControllerIphone switchFX:4 change:-1];
}
-(void)key5Pressed {
    [detailViewControllerIphone switchFX:5 change:1];
}
-(void)key5AltPressed {
    [detailViewControllerIphone switchFX:5 change:-1];
}
-(void)key6Pressed {
    [detailViewControllerIphone switchFX:6 change:1];
}
-(void)key6AltPressed {
    [detailViewControllerIphone switchFX:6 change:-1];
}
-(void)key7Pressed {
    [detailViewControllerIphone switchFX:7 change:1];
}
-(void)key7AltPressed {
    [detailViewControllerIphone switchFX:7 change:-1];
}
-(void)key8Pressed {
    [detailViewControllerIphone switchFX:8 change:1];
}
-(void)key8AltPressed {
    [detailViewControllerIphone switchFX:8 change:-1];
}
-(void)key9Pressed {
    [detailViewControllerIphone switchFX:9 change:1];
}
-(void)key9AltPressed {
    [detailViewControllerIphone switchFX:9 change:-1];
}
-(void)key0Pressed {
    [detailViewControllerIphone switchFX:0 change:1];
}
-(void)key0AltPressed {
    [detailViewControllerIphone switchFX:0 change:-1];
}
- (void)enterPressed{
    [detailViewControllerIphone oglViewSwitchFS];
}
- (void)keyVPressed{
    [detailViewControllerIphone mdSwitchVolBars];
}
- (void)keyBPressed{
    [detailViewControllerIphone mdSwitchFixedBar];
}
- (void)keyIPressed{
    [detailViewControllerIphone mdShowMusicInfo];
    [detailViewControllerIphone mdInfoFX];
}
- (void)keyIAltPressed{
    
}
- (void)keyMPressed{
    [detailViewControllerIphone mdSwitchSpectrumBloom:1];
}
- (void)keyMAltPressed{
    [detailViewControllerIphone mdSwitchSpectrumBloom:-1];
}
- (void)keyOPressed{
    [detailViewControllerIphone mdSwitchLandscapeBloom:1];
}
- (void)keyOAltPressed{
    [detailViewControllerIphone mdSwitchLandscapeBloom:-1];
}
- (void)keyFPressed{
    [detailViewControllerIphone mdSwitchModPatternFont:1];
}
- (void)keyFAltPressed{
    [detailViewControllerIphone mdSwitchModPatternFont:-1];
}
- (void)keySPressed{
    [detailViewControllerIphone mdSwitchModPatternFontSize:1];
}
- (void)keySAltPressed{
    [detailViewControllerIphone mdSwitchModPatternFontSize:-1];
}
- (void)keyTPressed{
    [detailViewControllerIphone mdSwitchModPatternTheme:1];
}
- (void)keyTAltPressed{
    [detailViewControllerIphone mdSwitchModPatternTheme:-1];
}
- (void)keyEPressed{
    [detailViewControllerIphone oglButtonPushed];
}
- (void)keyNPressed{
    [detailViewControllerIphone mdNextPreset];
}
- (void)keyPPressed{
    [detailViewControllerIphone mdPrevPreset];
}
- (void)keyHPressed{
    [detailViewControllerIphone mdSwitchFPSHud];
}
- (void)keyLPressed{
    [detailViewControllerIphone mdSwitchLockStatusPreset];
}
- (void)keyAPressed{
    [detailViewControllerIphone mdChangeFavoriteStatusPreset:0];
}
- (void)keyESCPressed{
    [detailViewControllerIphone mdOpenCloseMenu];
}
- (void)keyDeletePressed{
    [detailViewControllerIphone mdBackAction];
}
- (void)keyQPressed {
    [detailViewControllerIphone mdTestAsyncLoad];
}
/*
- (UIViewController *) getVisibleViewControllerFrom:(UIViewController *) vc {
    if ([vc isKindOfClass:[UINavigationController class]]) {
        return [self getVisibleViewControllerFrom:[((UINavigationController *) vc) visibleViewController]];
    } else if ([vc isKindOfClass:[UITabBarController class]]) {
        return [self getVisibleViewControllerFrom:[((UITabBarController *) vc) selectedViewController]];
    } else {
        if (vc.presentedViewController) {
            return [self getVisibleViewControllerFrom:vc.presentedViewController];
        } else {
            return vc;
        }
    }
}

- (UIViewController *)visibleViewController {
    UIViewController *rootViewController = self;//.rootViewController;
    return [self getVisibleViewControllerFrom:rootViewController];
}*/

- (void)keyTabPressed{
    UIViewController *currentVC=[self visibleViewController:self];
    if (currentVC) {
        if ([currentVC respondsToSelector:@selector(goPlayer)]) [currentVC performSelector:@selector(goPlayer)];
    }
}

-(void)leftPressed {
    [detailViewControllerIphone jumpSeekBwd];
}
-(void)rightPressed {
    [detailViewControllerIphone jumpSeekFwd];
}
-(void)leftCmdPressed {
    [detailViewControllerIphone playPrev];
}
-(void)rightCmdPressed {
    [detailViewControllerIphone playNext];
}
-(void)leftAltPressed {
    if ((detailViewControllerIphone.mplayer.mod_subsongs>1)&&
        (detailViewControllerIphone.mplayer.mod_currentsub>detailViewControllerIphone.mplayer.mod_minsub)&&(detailViewControllerIphone.mOnlyCurrentSubEntry==0))
        [detailViewControllerIphone playPrevSub]; //should handle sub ?
    else {//no more subsongs, check if within an archive to play prev entry
        if ([detailViewControllerIphone.mplayer isArchive]&&([detailViewControllerIphone.mplayer getArcEntriesCnt]>1)&&([detailViewControllerIphone.mplayer getArcIndex]>0)&&(detailViewControllerIphone.mOnlyCurrentEntry==0)) {
            [detailViewControllerIphone.mplayer selectPrevArcEntry];
            [detailViewControllerIphone play_loadArchiveModule];
        } else [detailViewControllerIphone play_prevEntry];
    }
}
-(void)rightAltPressed {
    //1st check if there are more subsongs
    if ((detailViewControllerIphone.mplayer.mod_subsongs>1)&&
        (detailViewControllerIphone.mplayer.mod_currentsub<detailViewControllerIphone.mplayer.mod_maxsub)&&(detailViewControllerIphone.mOnlyCurrentSubEntry==0))
        [detailViewControllerIphone playNextSub];
    else {
        //no more subsongs, check if within an archive to play next entry
        
        if ([detailViewControllerIphone.mplayer isArchive]&&([detailViewControllerIphone.mplayer getArcEntriesCnt]>1)&&(detailViewControllerIphone.mOnlyCurrentEntry==0)) {
            if ([detailViewControllerIphone.mplayer selectNextArcEntry]<0) [detailViewControllerIphone play_nextEntry];
            else [detailViewControllerIphone play_loadArchiveModule];
        } else [detailViewControllerIphone play_nextEntry];
    }
}

-(void)upPressed {
    [detailViewControllerIphone restartCurrent];
}
-(void)downPressed {
    [self rightAltPressed];
}
-(void)spacePressed {
    if (detailViewControllerIphone.mPaused) [detailViewControllerIphone playPushed:nil];
    else [detailViewControllerIphone pausePushed:nil];
}

#pragma mark - Press Events

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    bool _dontForwardEvent=false;
    if (@available(iOS 13.4, *)) {
        for (UIPress *press in presses) {
            UIKey *key=press.key;
            if (key.keyCode==UIKeyboardHIDUsageKeyboardRightShift) {
                [detailViewControllerIphone mdShiftMode:0];
                shiftPressedR=1;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardLeftShift) {
                shiftPressedL=1;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardRightArrow) {
                move_cursorR=1;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardLeftArrow) {
                move_cursorL=1;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardDeleteForward) {
                keyDel=1;
            }
        }
    }
    if (!_dontForwardEvent) [super pressesEnded:presses withEvent:event];
}
- (void)pressesBegan:(NSSet<UIPress *> *)presses
           withEvent:(UIPressesEvent *)event {
    bool _dontForwardEvent=false;
    if (@available(iOS 13.4, *)) {
        for (UIPress *press in presses) {
            UIKey *key=press.key;
            if (key.keyCode==UIKeyboardHIDUsageKeyboardRightShift) {
                [detailViewControllerIphone mdShiftMode:1];
                shiftPressedR=2;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardLeftShift) {
                shiftPressedL=2;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardRightArrow) {
                move_cursorR=2;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardLeftArrow) {
                move_cursorL=2;
            }
            if (key.keyCode==UIKeyboardHIDUsageKeyboardDeleteForward) {
                keyDel=2;
            }
        }
    }
    if (!_dontForwardEvent) [super pressesBegan:presses withEvent:event];
}
#pragma mark - Remote Control

- (void)remoteControlReceivedWithEvent:(UIEvent *)event {
    if (event.subtype == UIEventSubtypeRemoteControlTogglePlayPause) {
        if (detailViewControllerIphone.mPaused) {
            detailViewControllerIphone.mPaused=0;
            [detailViewControllerIphone.mplayer Pause:NO];
        } else {
            detailViewControllerIphone.mPaused=1;
            [detailViewControllerIphone.mplayer Pause:YES];
        }
    }
    if (event.subtype == UIEventSubtypeRemoteControlPlay) {
        if (detailViewControllerIphone.mPaused) {
            detailViewControllerIphone.mPaused=0;
            [detailViewControllerIphone.mplayer Pause:NO];
        }
    }
    if (event.subtype == UIEventSubtypeRemoteControlPause) {
        if (detailViewControllerIphone.mPaused==0) {
            detailViewControllerIphone.mPaused=1;
            [detailViewControllerIphone.mplayer Pause:YES];
        }
    }
    if (event.subtype == UIEventSubtypeRemoteControlStop) {
        if (detailViewControllerIphone.mPaused==0) {
            detailViewControllerIphone.mPaused=1;
            [detailViewControllerIphone.mplayer Pause:YES];
        }
    }
    if (event.subtype == UIEventSubtypeRemoteControlNextTrack) {
        //1st check if there are more subsongs
        if ((detailViewControllerIphone.mplayer.mod_subsongs>1)&&
            (detailViewControllerIphone.mplayer.mod_currentsub<detailViewControllerIphone.mplayer.mod_maxsub)&&(detailViewControllerIphone.mOnlyCurrentSubEntry==0))
            [detailViewControllerIphone playNextSub];
        else {
            //no more subsongs, check if within an archive to play next entry
            
            if ([detailViewControllerIphone.mplayer isArchive]&&([detailViewControllerIphone.mplayer getArcEntriesCnt]>1)&&(detailViewControllerIphone.mOnlyCurrentEntry==0)) {
                if ([detailViewControllerIphone.mplayer selectNextArcEntry]<0) [detailViewControllerIphone play_nextEntry];
                else [detailViewControllerIphone play_loadArchiveModule];
            } else [detailViewControllerIphone play_nextEntry];
        }
    }
    if (event.subtype == UIEventSubtypeRemoteControlPreviousTrack) {
        //1st check if there are more subsongs
        if ((detailViewControllerIphone.mplayer.mod_subsongs>1)&&
            (detailViewControllerIphone.mplayer.mod_currentsub>detailViewControllerIphone.mplayer.mod_minsub)&&(detailViewControllerIphone.mOnlyCurrentSubEntry==0))
            [detailViewControllerIphone playPrevSub]; //should handle sub ?
        else {//no more subsongs, check if within an archive to play prev entry
            if ([detailViewControllerIphone.mplayer isArchive]&&([detailViewControllerIphone.mplayer getArcEntriesCnt]>1)&&([detailViewControllerIphone.mplayer getArcIndex]>0)&&(detailViewControllerIphone.mOnlyCurrentEntry==0)) {
                [detailViewControllerIphone.mplayer selectPrevArcEntry];
                [detailViewControllerIphone play_loadArchiveModule];
            } else [detailViewControllerIphone play_prevEntry];
        }
    }
    
    /*UIEventSubtypeRemoteControlBeginSeekingBackward = 106,
    UIEventSubtypeRemoteControlEndSeekingBackward   = 107,
    UIEventSubtypeRemoteControlBeginSeekingForward  = 108,
    UIEventSubtypeRemoteControlEndSeekingForward    = 109,*/
}


- (BOOL)canBecomeFirstResponder {
    return YES;
}


@end

