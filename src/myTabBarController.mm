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

#import "SceneDelegate.h"


extern int shiftPressedL,shiftPressedR;
extern int move_cursorL,move_cursorR,keyDel;

@implementation myTabBarController

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

//- (BOOL)shouldAutorotate {
//    [self shouldAutorotateToInterfaceOrientation:self.interfaceOrientation];
//    return YES;
//}
//
//- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
//	return YES;
//}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    self.navigationController.delegate = self;
    
    [self setNeedsStatusBarAppearanceUpdate];
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

- (void)viewDidLoad {
    START_PROFILE
	[super viewDidLoad];
    self.navigationController.delegate = self;
    
    //self.view.backgroundColor = [UIColor clearColor];
    
    if (@available(iOS 18.0, *)) {
        if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad) {
            self.traitOverrides.horizontalSizeClass = UIUserInterfaceSizeClassCompact;
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
    
    
    
    //Faster loading for debug
#ifdef DEBUG_MODIZER
    //[window addSubview:[animatedLaunchVC view]];
#else
//    [window addSubview:[animatedLaunchVC view]];
#endif
    
    END_PROFILE
}

- (void)showAnimatedLaunchOverlay {
    if (self.animatedLaunchVC != nil) { return; }

    AnimatedLaunchVC *vc = [[AnimatedLaunchVC alloc] initWithNibName:@"AnimatedLaunch" bundle:[NSBundle mainBundle]];
    vc.localBrowserVC = self.rootViewControllerIphone;

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
        
        [self showAnimatedLaunchOverlay];
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
- (void)keyIPressed{
    [detailViewControllerIphone mdInfoFX];
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

