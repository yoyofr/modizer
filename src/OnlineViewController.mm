//
//  OnlineViewController.mm
//  modizer
//
//  Created by Yohann Magnien on 29/07/13.
//
//


enum {
    ONLINE_COLLECTIONS_MODLAND=0,
    ONLINE_COLLECTIONS_AMP,
    ONLINE_COLLECTIONS_HVSC,
    ONLINE_COLLECTIONS_CGSC,
    ONLINE_COLLECTIONS_ASMA,
    ONLINE_COLLECTIONS_JOSHW,
    ONLINE_COLLECTIONS_VGMRips,
    ONLINE_COLLECTIONS_SNESM,
    ONLINE_COLLECTIONS_SMSP,
    ONLINE_COLLECTIONS_ZXART,
    ONLINE_COLLECTIONS_NUMBER
};

#define GET_NB_ENTRIES 1
//#define NB_MODLAND_ENTRIES 319746
//#define NB_HVSC_ENTRIES 43856
//#define NB_ASMA_ENTRIES 4630

#define WEBLINKS_VGM_NB 6
#define WEBLINKS_MODS_NB 6
#define WEBLINKS_Others_NB 2
NSString *weblinks_VGM[WEBLINKS_VGM_NB][2]={
    //VGM & assimilated
    {@"http://snesmusic.org/pmh/",@"Portable Music History"},
    {@"http://snesmusic.org/cmh/",@"Console Music History"},
    {@"http://www.midishrine.com/",@"Midishrine"},
    {@"https://www.vgmusic.com/",@"VGMusic"},
    {@"http://www.mirsoft.info/gamemods-archive.php",@"Mirsoft MODS"},
    {@"http://www.mirsoft.info/gamemids-archive.php",@"Mirsoft Midis"}
};

NSString *weblinks_MODS[WEBLINKS_MODS_NB][2]={
    //MODS & Chiptunes
    {@"https://modarchive.org/",@"The Mod Archive"},
    {@"https://opl.wafflenet.com/",@"The OPL Archive"},
    {@"http://2a03.free.fr/",@"2A03 - Chiptunes"},
    {@"http://sc68.atari.org/musics.html",@"Atari ST - SC68"},
    {@"http://sndh.atari.org",@"Atari ST SNDH Archive"},
    {@"https://www.exotica.org.uk/wiki/Main_Page",@"Exotica"}
};
NSString *weblinks_Others[WEBLINKS_Others_NB][2]={
    //MIDIs
    {@"http://www.lvbeethoven.fr/Midi/index_En.html",@"Beethoven Midis"},
    {@"http://www.kunstderfuge.com/",@"Classical music Midis"}
    
};

#import "OnlineViewController.h"
#import "TTFadeAnimator.h"
#import "Reachability.h"


@interface OnlineViewController ()

@end

@implementation OnlineViewController

@synthesize tableView;
@synthesize downloadViewController,webBrowser,collectionViewController,detailViewController;
@synthesize mNbMODLANDFileEntries,mNbHVSCFileEntries,mNbCGSCFileEntries,mNbASMAFileEntries;
@synthesize waitingView,waitingViewPlayer;

#include "MiniPlayerImplementTableView.h"

//- (id)initWithStyle:(UITableViewStyle)style
//{
//    self = [super initWithStyle:style];
//    if (self) {
//        // Custom initialization
//    }
//    return self;
//}

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////
#include "AlertsCommonFunctions.h"

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
    if (!self.webBrowser) self.webBrowser = [self findChildOfClass:[WebBrowser class] inTabBarController:tbc];
}


- (void)viewDidLoad
{
    START_PROFILE
    [super viewDidLoad];
    
    if ([NSProcessInfo processInfo].isiOSAppOnMac) {
        self.hidesBottomBarWhenPushed = YES;
    } else if (@available(iOS 18.0, *)) {
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
            self.hidesBottomBarWhenPushed = YES;
        }
    }
    
    [self loadControllers];
    
    self.navigationController.delegate = self;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    /////////////////////////////////////
    // Waiting view
    /////////////////////////////////////
    waitingView = [[WaitingView alloc] init];
    waitingView.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingView];
    waitingView.hidden=TRUE;
    
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
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    collectionViewController=nil;
    
    self.tableView.rowHeight = 40;
    
#ifdef GET_NB_ENTRIES
    mNbMODLANDFileEntries=DBHelper::getNbMODLANDFilesEntries();
    mNbHVSCFileEntries=DBHelper::getNbHVSCFilesEntries();
    mNbCGSCFileEntries=DBHelper::getNbCGSCFilesEntries();
    mNbASMAFileEntries=DBHelper::getNbASMAFilesEntries();
#else
    mNbMODLANDFileEntries=NB_MODLAND_ENTRIES;
    mNbHVSCFileEntries=NB_HVSC_ENTRIES;
    mNbASMAFileEntries=NB_ASMA_ENTRIES;
#endif
    
    
    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
    
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
    
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:NOW_PLAYING_ICON] style:UIBarButtonItemStylePlain target:self action:@selector(goPlayer)];
    self.navigationItem.rightBarButtonItem = item;
    
    //    [self.tableView registerClass:[UITableViewCell class] forCellReuseIdentifier:@"Cell"];
    //self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
    
    self.tableView.sectionHeaderHeight = 30;
    END_PROFILE
}

-(void) traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    [self.tableView reloadData];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleDefault;
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [self.tableView reloadData];
    [miniplayerVC viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}


-(void) viewWillAppear:(BOOL)animated {
//    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
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
    
    [self.webBrowser loadBookmarks];
    
    [self.tableView reloadData];
    collectionViewController=nil;
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)dealloc {
    [waitingView removeFromSuperview];
    waitingView=nil;
}

-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:YES];
    else {
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"")];
    }
    
}

#pragma mark - Table view data source

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    UIView *customView = [[UIView alloc] initWithFrame: CGRectMake(0.0, 0.0, tableView.bounds.size.width, 30.0)];
    
    float scaleFactor=0.6f;
    
    if (darkMode) customView.backgroundColor = [UIColor colorWithRed: MDZ_TABVIEW_HEADER_R0*scaleFactor green: MDZ_TABVIEW_HEADER_G0*scaleFactor blue: MDZ_TABVIEW_HEADER_B0*scaleFactor alpha: 1.0f];
    else customView.backgroundColor = [UIColor colorWithRed: MDZ_TABVIEW_HEADER_R0 green: MDZ_TABVIEW_HEADER_G0 blue: MDZ_TABVIEW_HEADER_B0 alpha: 1.0f];
    
    CALayer *layerU = [CALayer layer];
    layerU.frame = CGRectMake(0.0, 0.0, tableView.bounds.size.width, 1.0);

    if (darkMode) layerU.backgroundColor = [[UIColor colorWithRed: MDZ_TABVIEW_HEADER_R1*scaleFactor green: MDZ_TABVIEW_HEADER_G1*scaleFactor blue: MDZ_TABVIEW_HEADER_B1*scaleFactor alpha: 1.00] CGColor];
    else layerU.backgroundColor = [[UIColor colorWithRed: MDZ_TABVIEW_HEADER_R1 green: MDZ_TABVIEW_HEADER_G1 blue: MDZ_TABVIEW_HEADER_B1 alpha: 1.00] CGColor];
    
    [customView.layer insertSublayer:layerU atIndex:0];
    
    CAGradientLayer *gradient = [CAGradientLayer layer];
    gradient.frame = CGRectMake(0.0, 1.0, tableView.bounds.size.width, 28.0);
    
    if (darkMode) gradient.colors = [NSArray arrayWithObjects:(id)[[UIColor colorWithRed: MDZ_TABVIEW_HEADER_R2*scaleFactor green: MDZ_TABVIEW_HEADER_G2*scaleFactor blue: MDZ_TABVIEW_HEADER_B2*scaleFactor alpha: 1.00] CGColor],
                                     (id)[[UIColor colorWithRed: MDZ_TABVIEW_HEADER_R1*scaleFactor green: MDZ_TABVIEW_HEADER_G1*scaleFactor blue: MDZ_TABVIEW_HEADER_B1*scaleFactor  alpha: 1.00] CGColor], nil];
    else gradient.colors = [NSArray arrayWithObjects:(id)[[UIColor colorWithRed: MDZ_TABVIEW_HEADER_R2 green: MDZ_TABVIEW_HEADER_G2 blue: MDZ_TABVIEW_HEADER_B2 alpha: 1.00] CGColor],
                            (id)[[UIColor colorWithRed: MDZ_TABVIEW_HEADER_R1 green: MDZ_TABVIEW_HEADER_G1 blue: MDZ_TABVIEW_HEADER_B1  alpha: 1.00] CGColor], nil];
    
    [customView.layer insertSublayer:gradient atIndex:0];
    
    CALayer *layerD = [CALayer layer];
    layerD.frame = CGRectMake(0.0, 23.0, tableView.bounds.size.width, 1.0);
    
    if (darkMode) layerD.backgroundColor = [[UIColor colorWithRed: MDZ_TABVIEW_HEADER_R2*scaleFactor green: MDZ_TABVIEW_HEADER_G2*scaleFactor blue: MDZ_TABVIEW_HEADER_B2*scaleFactor alpha: 1.00] CGColor];
    else layerD.backgroundColor = [[UIColor colorWithRed: MDZ_TABVIEW_HEADER_R2 green: MDZ_TABVIEW_HEADER_G2 blue: MDZ_TABVIEW_HEADER_B2 alpha: 1.00] CGColor];
    
    [customView.layer insertSublayer:layerD atIndex:0];
    
    UIButton *buttonLabel                  = [UIButton buttonWithType: UIButtonTypeCustom];
    buttonLabel.titleLabel.font            = [UIFont boldSystemFontOfSize: 20];
    buttonLabel.titleLabel.shadowOffset    = CGSizeMake (0.0, 1.0);
    buttonLabel.titleLabel.lineBreakMode   = (NSLineBreakMode)UILineBreakModeTailTruncation;
    //	buttonLabel.titleLabel.shadowOffset    = CGSizeMake (1.0, 0.0);
    buttonLabel.frame=CGRectMake(8, 0.0, tableView.bounds.size.width-8, 30);
    buttonLabel.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
    buttonLabel.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    
    NSString *lbl;
    switch (section) {
        case 0:
            lbl= NSLocalizedString(@"Collections", @"");
            break;
        case 1:
            lbl= NSLocalizedString(@"Browse Internet", @"");
            break;
        case 2:
            lbl= NSLocalizedString(@"Bookmarks", @"");
            break;
        case 3:
            lbl= NSLocalizedString(@"Games Music", @"");
            break;
        case 4:
            lbl= NSLocalizedString(@"Mods & chiptunes", @"");
            break;
        case 5:
            lbl= NSLocalizedString(@"Other", @"");
            break;
        default:
            lbl= nil;
            break;
    }
    
    [buttonLabel setTitle:lbl forState:UIControlStateNormal];
    
    [customView addSubview: buttonLabel];
    
    return customView;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return 6;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    switch (section) {
        case 0: //collections
            return ONLINE_COLLECTIONS_NUMBER;
        case 1: //internet
            return 2;
        case 2: //internet custom URLs
            return self.webBrowser.custom_url_count;
        case 3: //VGM
            return WEBLINKS_VGM_NB;
        case 4: //MODS
            return WEBLINKS_MODS_NB;
        case 5: //OThers
            return WEBLINKS_Others_NB;
        default:
            return 0;
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    const NSInteger TOP_LABEL_TAG = 1001;
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    UILabel *topLabel;
    UILabel *bottomLabel;
    int cell_category=0;
    
    
    NSString *CellIdentifier = [NSString stringWithFormat:@"Cell%d",cell_category];
    
    if (forceReloadCells) {
        while ([tableView dequeueReusableCellWithIdentifier:CellIdentifier]) {}
        forceReloadCells=false;
    }
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    if (cell==nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
        cell.frame=CGRectMake(0,0,tableView.frame.size.width,40);
        
//        [cell setBackgroundColor:[UIColor clearColor]];
//        UIBackgroundConfiguration *backgroundConfig = [UIBackgroundConfiguration listGroupedCellConfiguration];
//        backgroundConfig.backgroundColor = [UIColor systemGroupedBackgroundColor];
//        cell.backgroundConfiguration = backgroundConfig;
        UIBackgroundConfiguration *backgroundConfig = [UIBackgroundConfiguration listGroupedCellConfiguration];
        backgroundConfig.backgroundColor = [UIColor systemGroupedBackgroundColor];
        cell.backgroundConfiguration = backgroundConfig;
        
        // Pour gérer automatiquement l'état sélectionné, utilise configurationUpdateHandler
        cell.configurationUpdateHandler = ^(UITableViewCell *cell, UICellConfigurationState *state) {
            UIBackgroundConfiguration *config = [cell backgroundConfiguration];
            if (state.selected || state.highlighted) {
                config.backgroundColor = [UIColor secondarySystemGroupedBackgroundColor];
                //config.backgroundColor = [UIColor systemBlueColor];
                config.cornerRadius = 8.0;
            } else {
                config.backgroundColor = [UIColor systemGroupedBackgroundColor];
                config.cornerRadius = 0.0;
            }
            [cell setBackgroundConfiguration:config];
        };
        
        //
        // Create the label for the top row of text
        //
        topLabel = [[UILabel alloc] init];
        [cell.contentView addSubview:topLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        topLabel.tag = TOP_LABEL_TAG;
        topLabel.backgroundColor = [UIColor clearColor];
        topLabel.font = [UIFont systemFontOfSize:17 weight:MDZ_UIFONT_WEIGHT];
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;;
        topLabel.opaque=TRUE;
        
        //
        // Create the label for the top row of text
        //
        bottomLabel = [[UILabel alloc] init];
        [cell.contentView addSubview:bottomLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        bottomLabel.tag = BOTTOM_LABEL_TAG;
        bottomLabel.backgroundColor = [UIColor clearColor];
        bottomLabel.font = [UIFont systemFontOfSize:12];
        //bottomLabel.font = [UIFont fontWithName:@"courier" size:12];
        bottomLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                   ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;;
        bottomLabel.opaque=TRUE;
        cell.accessoryView=nil;
        //cell.selectionStyle=UITableViewCellSelectionStyleGray;
    } else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
        
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
        
        bottomLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
    }
    float margin=MDZ_TABVIEW_SEPARATOR_MARGIN;
    cell.layoutMargins = UIEdgeInsetsMake(0, margin, 0, margin);
    cell.separatorInset = UIEdgeInsetsMake(0, margin, 0, margin);
    
    if (darkMode) {
        topLabel.textColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:1 green:1 blue:1 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.6 green:0.6 blue:0.6 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
    } else {
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0];
    }
    
    cell.frame=CGRectMake(0,0,tableView.frame.size.width,40);
    
    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                               0,
                               tableView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                               22);
    bottomLabel.frame = CGRectMake(1.0 * cell.indentationWidth,
                                   22,
                                   tableView.bounds.size.width -1.0 * cell.indentationWidth-32,
                                   18);
    bottomLabel.text=@""; //default value
    
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    
    
    // Configure the cell...
    switch (indexPath.section) {
        case 0: //collections
        {
            switch (indexPath.row) {
                case ONLINE_COLLECTIONS_MODLAND:topLabel.text=NSLocalizedString(@"MODLAND collection",@"");
                    bottomLabel.text=[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),mNbMODLANDFileEntries];
                    break;
                case ONLINE_COLLECTIONS_AMP:topLabel.text=NSLocalizedString(@"AMP collection",@"");
                    bottomLabel.text=NSLocalizedString(@"Amiga Music Preservation",@"");
                    break;
                case ONLINE_COLLECTIONS_HVSC:topLabel.text=NSLocalizedString(@"HVSC collection",@"");
                    bottomLabel.text=[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),mNbHVSCFileEntries];
                    break;
                case ONLINE_COLLECTIONS_CGSC:topLabel.text=NSLocalizedString(@"CGSC collection",@"");
                    bottomLabel.text=[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),mNbCGSCFileEntries];
                    break;
                case ONLINE_COLLECTIONS_ASMA:topLabel.text=NSLocalizedString(@"ASMA collection",@"");
                    bottomLabel.text=[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),mNbASMAFileEntries];
                    break;
                case ONLINE_COLLECTIONS_JOSHW:topLabel.text=NSLocalizedString(@"JOSHW collection",@"");
                    bottomLabel.text=NSLocalizedString(@"Thousands of entries",@"");
                    break;
                case ONLINE_COLLECTIONS_VGMRips:topLabel.text=NSLocalizedString(@"VGMRips collection",@"");
                    bottomLabel.text=NSLocalizedString(@"4K+ packs / 70K+ songs",@"");
                    break;
                case ONLINE_COLLECTIONS_SNESM:topLabel.text=NSLocalizedString(@"SNESmusic collection",@"");
                    bottomLabel.text=NSLocalizedString(@"1500+ sets / Super Nintendo/Famicom",@"");
                    break;
                case ONLINE_COLLECTIONS_SMSP:topLabel.text=NSLocalizedString(@"SMS Power! collection",@"");
                    bottomLabel.text=NSLocalizedString(@"Master System & Game gear sets",@"");
                    break;
                case ONLINE_COLLECTIONS_ZXART:topLabel.text=NSLocalizedString(@"ZXArt collection",@"");
                    bottomLabel.text=NSLocalizedString(@"ZX Spectrum",@"");
                    break;
            }
        }
            break;
        case 1: //internet
        {
            switch (indexPath.row) {
                case 0:topLabel.text=NSLocalizedString(@"Modizer World Charts",@"");
                    bottomLabel.text=NSLocalizedString(@"Browser_WorldCharts_SubKey",@"");
                    break;
                case 1:topLabel.text=NSLocalizedString(@"Web browser",@"");
                    bottomLabel.text=NSLocalizedString(@"Browser_Web_SubKey",@"");
                    break;
            }
        }
            break;
        case 2: //custom internet URLs
            topLabel.text=[self.webBrowser.custom_URL_name objectAtIndex:indexPath.row];
            bottomLabel.text=[self.webBrowser.custom_URL objectAtIndex:indexPath.row];
            break;
        case 3: //VGM
            topLabel.text=weblinks_VGM[indexPath.row][1];
            bottomLabel.text=weblinks_VGM[indexPath.row][0];
            break;
        case 4: //MODS & Chptunes
            topLabel.text=weblinks_MODS[indexPath.row][1];
            bottomLabel.text=weblinks_MODS[indexPath.row][0];
            break;
        case 5: //Others
            topLabel.text=weblinks_Others[indexPath.row][1];
            bottomLabel.text=weblinks_Others[indexPath.row][0];
            break;
    }
    
    return cell;
}

/*
 // Override to support conditional editing of the table view.
 - (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
 {
 // Return NO if you do not want the specified item to be editable.
 return YES;
 }
 */

#pragma mark - Table view delegate

-(void) adjustFrame:(UIViewController*)childController {
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
}

-(void) tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    Reachability* reach;
    // Navigation logic may go here. Create and push another view controller.
    
    switch (indexPath.section) {
        case 0: {//collection
            switch (indexPath.row) {
                case ONLINE_COLLECTIONS_MODLAND: //MODLAND
                    collectionViewController = [[RootViewControllerMODLAND alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"MODLAND";
                    // Set new directory
                    ((RootViewControllerMODLAND*)collectionViewController)->browse_depth = 1;
                    ((RootViewControllerMODLAND*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerMODLAND*)collectionViewController)->downloadViewController=downloadViewController;
//                    collectionViewController.view.frame=self.view.frame;
                    [self adjustFrame:collectionViewController];
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_HVSC: //HVSC
                    collectionViewController =[[RootViewControllerHVSC alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"HVSC";
                    // Set new directory
                    ((RootViewControllerHVSC*)collectionViewController)->browse_depth = 1;
                    ((RootViewControllerHVSC*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerHVSC*)collectionViewController)->downloadViewController=downloadViewController;
//                    collectionViewController.view.frame=self.view.frame;
                    [self adjustFrame:collectionViewController];
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_CGSC: //CGSC
                    collectionViewController = [[RootViewControllerCGSC alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"CGSC";
                    // Set new directory
                    ((RootViewControllerCGSC*)collectionViewController)->browse_depth = 1;
                    ((RootViewControllerCGSC*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerCGSC*)collectionViewController)->downloadViewController=downloadViewController;
//                    collectionViewController.view.frame=self.view.frame;
                    [self adjustFrame:collectionViewController];
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_ASMA: //ASMA
                    collectionViewController = [[RootViewControllerASMA alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"ASMA";
                    // Set new directory
                    ((RootViewControllerASMA*)collectionViewController)->browse_depth = 1;
                    ((RootViewControllerASMA*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerASMA*)collectionViewController)->downloadViewController=downloadViewController;
//                    collectionViewController.view.frame=self.view.frame;
                    [self adjustFrame:collectionViewController];
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_JOSHW: //JOSHW
                    reach = [Reachability reachabilityWithHostname:@"pc.joshw.info/"];
                        
                    if ([reach isReachable]) {
                        collectionViewController = [[RootViewControllerJoshWWebParser alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
                        //set new title
                        collectionViewController.title = @"JoshW";
                        // Set new directory
                        ((RootViewControllerJoshWWebParser*)collectionViewController)->browse_depth = 0;
                        ((RootViewControllerJoshWWebParser*)collectionViewController)->detailViewController=detailViewController;
                        ((RootViewControllerJoshWWebParser*)collectionViewController)->downloadViewController=downloadViewController;
                        
//                        collectionViewController.view.frame=self.view.frame;
                        [self adjustFrame:collectionViewController];
                        // And push the window
                        [self.navigationController pushViewController:collectionViewController animated:YES];
                    } else {
                       [self showAlertMsg:NSLocalizedString(@"Warning",@"")
                                  message:NSLocalizedString(@"No internet connectivity or website down.",@"")];
                   }
                    break;
                case ONLINE_COLLECTIONS_VGMRips: //VGMRips
                    reach = [Reachability reachabilityWithHostname:@"vgmrips.net"];
                        
                    if ([reach isReachable]) {
                        collectionViewController = [[RootViewControllerVGMRWebParser alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
                        //set new title
                        collectionViewController.title = @"VGMRips";
                        // Set new directory
                        ((RootViewControllerVGMRWebParser*)collectionViewController)->browse_depth = 0;
                        ((RootViewControllerVGMRWebParser*)collectionViewController)->detailViewController=detailViewController;
                        ((RootViewControllerVGMRWebParser*)collectionViewController)->downloadViewController=downloadViewController;
                        
//                        collectionViewController.view.frame=self.view.frame;
                        [self adjustFrame:collectionViewController];
                        // And push the window
                        [self.navigationController pushViewController:collectionViewController animated:YES];
                    } else {
                        [self showAlertMsg:NSLocalizedString(@"Warning",@"")
                                   message:NSLocalizedString(@"No internet connectivity or website down.",@"")];
                    }
                    break;
                case ONLINE_COLLECTIONS_AMP: //AMP
                    collectionViewController = [[RootViewControllerAMPWebParser alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"AMP";
                    // Set new directory
                    ((RootViewControllerAMPWebParser*)collectionViewController)->browse_depth = 0;
                    ((RootViewControllerAMPWebParser*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerAMPWebParser*)collectionViewController)->downloadViewController=downloadViewController;
                    
                    //collectionViewController.view.frame=self.view.frame;
                    [self adjustFrame:collectionViewController];
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_SNESM: //SnesMusic
                    //check if reachable
                    // Allocate a reachability object
                    reach = [Reachability reachabilityWithHostname:@"snesmusic.org/v2"];
                    
                    if ([reach isReachable]) {
                        collectionViewController = [[RootViewControllerSNESMWebParser alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
                        //set new title
                        collectionViewController.title = @"SNESmusic";
                        // Set new directory
                        ((RootViewControllerSNESMWebParser*)collectionViewController)->browse_depth = 0;
                        ((RootViewControllerSNESMWebParser*)collectionViewController)->detailViewController=detailViewController;
                        ((RootViewControllerSNESMWebParser*)collectionViewController)->downloadViewController=downloadViewController;
                        
//                        collectionViewController.view.frame=self.view.frame;
                        [self adjustFrame:collectionViewController];
                        // And push the window
                        [self.navigationController pushViewController:collectionViewController animated:YES];
                    } else {
                        [self showAlertMsg:NSLocalizedString(@"Warning",@"")
                                   message:NSLocalizedString(@"No internet connectivity or website down.",@"")];
                    }
                    break;
                
                case ONLINE_COLLECTIONS_SMSP: //SMS Power!
                    reach = [Reachability reachabilityWithHostname:@"www.smspower.org"];
                        
                    if ([reach isReachable]) {
                        collectionViewController = [[RootViewControllerSMSPWebParser alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
                        //set new title
                        collectionViewController.title = @"SMS Power!";
                        // Set new directory
                        ((RootViewControllerSMSPWebParser*)collectionViewController)->browse_depth = 0;
                        ((RootViewControllerSMSPWebParser*)collectionViewController)->detailViewController=detailViewController;
                        ((RootViewControllerSMSPWebParser*)collectionViewController)->downloadViewController=downloadViewController;
                        
//                        collectionViewController.view.frame=self.view.frame;
                        [self adjustFrame:collectionViewController];
                        // And push the window
                        [self.navigationController pushViewController:collectionViewController animated:YES];
                    } else {
                        [self showAlertMsg:NSLocalizedString(@"Warning",@"")
                                   message:NSLocalizedString(@"No internet connectivity or website down.",@"")];
                    }
                    break;
                case ONLINE_COLLECTIONS_ZXART: //SMS Power!
                    reach = [Reachability reachabilityWithHostname:@"zxart.ee"];
                        
                    if ([reach isReachable]) {
                        collectionViewController = [[RootViewControllerZXArtWebParser alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
                        //set new title
                        collectionViewController.title = @"ZXArt";
                        // Set new directory
                        ((RootViewControllerZXArtWebParser*)collectionViewController)->browse_depth = 0;
                        ((RootViewControllerZXArtWebParser*)collectionViewController)->detailViewController=detailViewController;
                        ((RootViewControllerZXArtWebParser*)collectionViewController)->downloadViewController=downloadViewController;
                        
//                        collectionViewController.view.frame=self.view.frame;
                        [self adjustFrame:collectionViewController];
                        // And push the window
                        [self.navigationController pushViewController:collectionViewController animated:YES];
                    } else {
                        [self showAlertMsg:NSLocalizedString(@"Warning",@"")
                                   message:NSLocalizedString(@"No internet connectivity or website down.",@"")];
                    }
                    break;
            }
        }
            break;
        case 1:{//internet
                switch (indexPath.row) {
                    case 0: //World Charts
                        reach = [Reachability reachabilityWithHostname:@"modizerdb.appspot.com"];
                        if ([reach isReachable]) {
                            [webBrowser loadWorldCharts];
                            [self.navigationController pushViewController:webBrowser animated:YES];
                        } else {
                            [self showAlertMsg:NSLocalizedString(@"Warning",@"")
                                       message:NSLocalizedString(@"No internet connectivity or website down.",@"")];
                        }
                        break;
                    case 1: //Internet
                        [webBrowser loadLastURL];
                        [self.navigationController pushViewController:webBrowser animated:YES];
                        break;
                }
            }
            break;
        case 2: //internet custom URLs
            reach = [Reachability reachabilityWithHostName:webBrowser.custom_URL[indexPath.row]];
            if ([reach isReachable]) {
                [webBrowser goToURLwithLoad:webBrowser.custom_URL[indexPath.row]];
                [self.navigationController pushViewController:webBrowser animated:YES];
            }
            else {
               [self showAlertMsg:NSLocalizedString(@"Warning",@"")
                          message:NSLocalizedString(@"No internet connectivity or website down.",@"")];
           }
            break;
        case 3: //VGM
            reach = [Reachability reachabilityWithHostName:weblinks_VGM[indexPath.row][0]];
            if ([reach isReachable]) {
                [webBrowser goToURLwithLoad:weblinks_VGM[indexPath.row][0]];
                [self.navigationController pushViewController:webBrowser animated:YES];
            } else {
                [self showAlertMsg:NSLocalizedString(@"Warning",@"")
                           message:NSLocalizedString(@"No internet connectivity or website down.",@"")];
            }
            break;
        case 4: //MODS & Chiptunes
            reach = [Reachability reachabilityWithHostName:weblinks_MODS[indexPath.row][0]];
            if ([reach isReachable]) {
                [webBrowser goToURL:weblinks_MODS[indexPath.row][0]];
                [self.navigationController pushViewController:webBrowser animated:YES];
            } else {
                [self showAlertMsg:NSLocalizedString(@"Warning",@"")
                           message:NSLocalizedString(@"No internet connectivity or website down.",@"")];
            }
            break;
        case 5: //Others
            reach = [Reachability reachabilityWithHostName:weblinks_Others[indexPath.row][0]];
            if ([reach isReachable]) {
                [webBrowser goToURL:weblinks_Others[indexPath.row][0]];
                [self.navigationController pushViewController:webBrowser animated:YES];
            } else {
                [self showAlertMsg:NSLocalizedString(@"Warning",@"")
                           message:NSLocalizedString(@"No internet connectivity or website down.",@"")];
            }
            break;
    }
}

-(void) refreshViewAfterDownload {
    [tableView reloadData];
    if (collectionViewController) [(RootViewControllerASMA*)collectionViewController refreshViewAfterDownload];
    //TODO: review how to manage: build a new virtual class ?
}

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
        [self updateMiniPlayer];
    }
}


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
