//
//  VoiceCommandsViewController.mm
//  modizer
//
//  Created for Siri voice commands display
//

#import "VoiceCommandsViewController.h"

@implementation VoiceCommandsViewController

@synthesize detailViewController, tableView, commandCategories, commandsByCategory;

#include "MiniPlayerImplementTableView.h"
#include "AlertsCommonFunctions.h"

-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:YES];
    else {
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"")];
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////

-(void)viewDidDisappear:(BOOL)animated {
    [repeatingTimer invalidate];
    repeatingTimer = nil;

    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView removeObserver:self
                                          forKeyPath:observedSelector
                                             context:LoadingProgressObserverContext];
    [super viewDidDisappear:animated];
}

- (id)findChildOfClass:(Class)cls inTabBarController:(UITabBarController *)tbc {
    for (UIViewController *vc in tbc.viewControllers) {
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

- (void)loadControllers {
    UIWindow *window=[UIApplication sharedApplication].windows.firstObject;
    if (!window) {
        window = [UIApplication sharedApplication].keyWindow;
    }
    UITabBarController *tbc = (UITabBarController *)window.rootViewController;
    if (![tbc isKindOfClass:[UITabBarController class]]) {
        MDZELog("[VoiceCommandsVC] Unexpected root VC: %@", NSStringFromClass([window.rootViewController class]));
        return;
    }
    if (!self.detailViewController) self.detailViewController = [self findChildOfClass:[DetailViewControllerIphone class] inTabBarController:tbc];
}

- (void)setupVoiceCommands {
    // Organize Siri voice commands by category
    // Uses exact phrases from ModizerAppIntents.swift -> ModizerShortcuts
    // Each command shows both phrases when available (phrase 1 / phrase 2)

    self.commandCategories = @[
        NSLocalizedString(@"Voice Commands: Playback", @""),
        NSLocalizedString(@"Voice Commands: Playlists", @""),
        NSLocalizedString(@"Voice Commands: Modes", @""),
        NSLocalizedString(@"Voice Commands: Info", @"")
    ];

    self.commandsByCategory = @[
        // Playback commands - from ModizerShortcuts AppShortcut phrases
        @[
            @{@"phrase1": NSLocalizedString(@"Play some music in Modizer", @""),
              @"phrase2": NSLocalizedString(@"Start random playlist in Modizer", @"")},
            @{@"phrase1": NSLocalizedString(@"Play favorites in Modizer", @""),
              @"phrase2": NSLocalizedString(@"Start favorites playlist in Modizer", @"")},
            @{@"phrase1": NSLocalizedString(@"Play most played in Modizer", @""),
              @"phrase2": NSLocalizedString(@"Start most played playlist in Modizer", @"")},
        ],
        // Playlist commands
        @[
            @{@"phrase1": NSLocalizedString(@"Play a playlist in Modizer", @""),
              @"phrase2": NSLocalizedString(@"Start a playlist in Modizer", @"")},
        ],
        // Modes commands
        @[
            @{@"phrase1": NSLocalizedString(@"Toggle shuffle mode in Modizer", @""),
              @"phrase2": NSLocalizedString(@"Change shuffle mode in Modizer", @"")},
            @{@"phrase1": NSLocalizedString(@"Change loop mode in Modizer", @""),
              @"phrase2": NSLocalizedString(@"Toggle repeat in Modizer", @"")},
        ],
        // Info commands
        @[
            @{@"phrase1": NSLocalizedString(@"What's playing in Modizer", @""),
              @"phrase2": NSLocalizedString(@"Current song in Modizer", @"")},
        ]
    ];
}

- (void)viewDidLoad {
    START_PROFILE
    [super viewDidLoad];

    [self loadControllers];
    [self setupVoiceCommands];

    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;

    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;

    // Create table view programmatically
    self.tableView = [[UITableView alloc] initWithFrame:self.view.bounds style:UITableViewStyleGrouped];
    self.tableView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.tableView.delegate = self;
    self.tableView.dataSource = self;
    if (darkMode) self.tableView.backgroundColor = [UIColor blackColor];
    else self.tableView.backgroundColor = [UIColor whiteColor];
    [self.view addSubview:self.tableView];

    /////////////////////////////////////
    // Waiting view
    /////////////////////////////////////
    waitingView = [[WaitingView alloc] init];
    waitingView.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingView];
    waitingView.hidden=TRUE;

    NSDictionary *views = NSDictionaryOfVariableBindings(waitingView);
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingView(150)]" options:0 metrics:nil views:views]];
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingView(150)]" options:0 metrics:nil views:views]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];

    waitingViewPlayer = [[WaitingView alloc] init];
    waitingViewPlayer.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingViewPlayer];

    views = NSDictionaryOfVariableBindings(waitingViewPlayer);
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];

    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:NOW_PLAYING_ICON] style:UIBarButtonItemStylePlain target:self action:@selector(goPlayer)];
    self.navigationItem.rightBarButtonItem = item;

    END_PROFILE
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) {
        if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
        else self.tableView.backgroundColor=[UIColor whiteColor];
        [self.tableView reloadData];
    }
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
    
#if TARGET_OS_MACCATALYST
    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:(NSString *)NSLocalizedString(@"Voices commands aren't supported in MacOS.","")];
#endif

}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleDefault;
}

- (void)viewWillAppear:(BOOL)animated {
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) {
        if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
        else self.tableView.backgroundColor=[UIColor whiteColor];
    }

    if ([detailViewController mPlaylist_size]>0) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    } else {
        wasMiniPlayerOn=false;
        [self hideMiniPlayer];
    }

    [waitingViewPlayer resetCancelStatus];
    waitingViewPlayer.hidden=detailViewController.waitingView.hidden;
    waitingViewPlayer.btnStopCurrentAction.hidden=detailViewController.waitingView.btnStopCurrentAction.hidden;
    waitingViewPlayer.progressView.progress=detailViewController.waitingView.progressView.progress;
    waitingViewPlayer.progressView.hidden=detailViewController.waitingView.progressView.hidden;
    waitingViewPlayer.lblTitle.text=[NSString stringWithString:detailViewController.waitingView.lblTitle.text];
    waitingViewPlayer.lblDetail.text=[NSString stringWithString:detailViewController.waitingView.lblDetail.text];

    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView addObserver:self
                                       forKeyPath:observedSelector
                                          options:NSKeyValueObservingOptionInitial
                                          context:LoadingProgressObserverContext];

    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.20f target:self selector:@selector(updateLoadingInfos:) userInfo:nil repeats: YES];

    [self hideWaiting];

    [super viewWillAppear:animated];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return [self.commandCategories count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    NSArray *commands = self.commandsByCategory[section];
    return [commands count];
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return self.commandCategories[section];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 60;  // Taller cells to accommodate 2 phrases
}

- (UITableViewCell *)tableView:(UITableView *)aTableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"VoiceCommandCell";
    const NSInteger TOP_LABEL_TAG = 2001;
    const NSInteger BOTTOM_LABEL_TAG = 2002;
    UILabel *topLabel;
    UILabel *bottomLabel;

    UITableViewCell *cell = [aTableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];

        [cell setBackgroundColor:[UIColor clearColor]];

        NSString *imgFile=(darkMode?@"tabview_gradient40Black.png":@"tabview_gradient40.png");
        UIImage *image = [UIImage imageNamed:imgFile];
        UIImageView *imageView = [[UIImageView alloc] initWithImage:image];
        imageView.contentMode = UIViewContentModeScaleToFill;
        cell.backgroundView = imageView;

        // Create the label for phrase 1 (top row)
        topLabel = [[UILabel alloc] init];
        [cell.contentView addSubview:topLabel];
        topLabel.tag = TOP_LABEL_TAG;
        topLabel.backgroundColor = [UIColor clearColor];
        topLabel.font = [UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];
        topLabel.lineBreakMode = NSLineBreakByTruncatingTail;
        topLabel.opaque = TRUE;

        // Create the label for phrase 2 (bottom row)
        bottomLabel = [[UILabel alloc] init];
        [cell.contentView addSubview:bottomLabel];
        bottomLabel.tag = BOTTOM_LABEL_TAG;
        bottomLabel.backgroundColor = [UIColor clearColor];
        bottomLabel.font = [UIFont systemFontOfSize:12];
        bottomLabel.lineBreakMode = NSLineBreakByTruncatingTail;
        bottomLabel.numberOfLines = 2;  // Allow 2 lines
        bottomLabel.opaque = TRUE;

        cell.selectionStyle = UITableViewCellSelectionStyleNone;
    } else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
    }

    if (darkMode) {
        topLabel.textColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.6 green:0.6 blue:0.6 alpha:1.0];
    } else {
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
    }

    topLabel.frame = CGRectMake(10, 8, aTableView.bounds.size.width - 20, 18);
    bottomLabel.frame = CGRectMake(10, 26, aTableView.bounds.size.width - 20, 28);

    // Get command data
    NSArray *commands = self.commandsByCategory[indexPath.section];
    NSDictionary *commandData = commands[indexPath.row];

    // Show both phrases
    NSString *phrase1 = commandData[@"phrase1"];
    NSString *phrase2 = commandData[@"phrase2"];

    topLabel.text = phrase1;
    bottomLabel.text = phrase2;

    return cell;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
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
            if (detailViewController.waitingView.lblTitle.text) {
                waitingViewPlayer.lblTitle.text=[NSString stringWithString:detailViewController.waitingView.lblTitle.text];
            }
            if (detailViewController.waitingView.lblDetail.text) {
                waitingViewPlayer.lblDetail.text=[NSString stringWithString:detailViewController.waitingView.lblDetail.text];
            }
        }
        waitingViewPlayer.hidden=wv.hidden;
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)dealloc {
    [waitingView removeFromSuperview];
    [waitingViewPlayer removeFromSuperview];
    waitingView=nil;
    waitingViewPlayer=nil;
}

@end
