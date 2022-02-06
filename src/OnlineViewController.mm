//
//  OnlineViewController.m
//  modizer
//
//  Created by Yohann Magnien on 29/07/13.
//
//

enum {
    ONLINE_COLLECTIONS_MODLAND=0,
    ONLINE_COLLECTIONS_HVSC,
    ONLINE_COLLECTIONS_ASMA,
    ONLINE_COLLECTIONS_JOSHW,
    ONLINE_COLLECTIONS_VGMRips,
    ONLINE_COLLECTIONS_P2612,
    ONLINE_COLLECTIONS_SNESM,
    ONLINE_COLLECTIONS_SMSP,
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
    {@"http://amp.dascene.net/",@"Amiga Music Preservation"},
    {@"http://2a03.free.fr/",@"2A03 - Chiptunes"},
    {@"http://sc68.atari.org/musics.html",@"Atari ST - SC68"},
    {@"http://sndh.atari.org/sndh/browser/index.php?dir=sndh_lf%2F",@"Atari ST SNDH Archive"},
    {@"https://www.exotica.org.uk/wiki/Main_Page",@"Exotica"}
};
NSString *weblinks_Others[WEBLINKS_Others_NB][2]={
    //MIDIs
    {@"http://www.lvbeethoven.fr/Midi/index_En.html",@"Beethoven Midis"},
    {@"http://www.kunstderfuge.com/",@"Classical music Midis"}

};

#import "OnlineViewController.h"
#import "TTFadeAnimator.h"


@interface OnlineViewController ()

@end

@implementation OnlineViewController

@synthesize tableView;
@synthesize downloadViewController,webBrowser,collectionViewController,detailViewController;
@synthesize mNbMODLANDFileEntries,mNbHVSCFileEntries,mNbASMAFileEntries;

#include "MiniPlayerImplementTableView.h"

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////


- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.navigationController.delegate = self;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    /////////////////////////////////////
    // Waiting view
    /////////////////////////////////////
    waitingView = [[WaitingView alloc] init];
    [self.view addSubview:waitingView];
    
    NSDictionary *views = NSDictionaryOfVariableBindings(waitingView);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingView(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingView(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    forceReloadCells=false;
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
    
    collectionViewController=nil;
    
    self.tableView.rowHeight = 40;
    
#ifdef GET_NB_ENTRIES
    mNbMODLANDFileEntries=DBHelper::getNbMODLANDFilesEntries();
    mNbHVSCFileEntries=DBHelper::getNbHVSCFilesEntries();
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
    
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithCustomView: btn];
    self.navigationItem.rightBarButtonItem = item;
    
    
//    [self.tableView registerClass:[UITableViewCell class] forCellReuseIdentifier:@"Cell"];
	//self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
}

-(void) traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    bool oldmode=darkMode;
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    [self.tableView reloadData];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleDefault;
}

-(void) viewWillAppear:(BOOL)animated {
    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    
    self.navigationController.delegate = self;
    
    bool oldmode=darkMode;
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
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
    
    [self hideWaiting];
    
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
        UIAlertView *nofileplaying=[[UIAlertView alloc] initWithTitle:@"Warning"
                                                               message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
        [nofileplaying show];
    }

}

#pragma mark - Table view data source

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    UIView *customView = [[UIView alloc] initWithFrame: CGRectMake(0.0, 0.0, tableView.bounds.size.width, 24.0)];
    
    if (darkMode) customView.backgroundColor = [UIColor colorWithRed: 1-0.7f green: 1-0.7f blue: 1-0.7f alpha: 1.0f];
    else customView.backgroundColor = [UIColor colorWithRed: 0.7f green: 0.7f blue: 0.7f alpha: 1.0f];
    
    CALayer *layerU = [CALayer layer];
    layerU.frame = CGRectMake(0.0, 0.0, tableView.bounds.size.width, 1.0);
    
    if (darkMode) layerU.backgroundColor = [[UIColor colorWithRed: 1-183.0f/255.0f green: 1-193.0f/255.0f blue: 1-199.0f/255.0f alpha: 1.00] CGColor];
    else layerU.backgroundColor = [[UIColor colorWithRed: 183.0f/255.0f green: 193.0f/255.0f blue: 199.0f/255.0f alpha: 1.00] CGColor];
    
    [customView.layer insertSublayer:layerU atIndex:0];
    
    CAGradientLayer *gradient = [CAGradientLayer layer];
    gradient.frame = CGRectMake(0.0, 1.0, tableView.bounds.size.width, 22.0);
    
    if (darkMode) gradient.colors = [NSArray arrayWithObjects:(id)[[UIColor colorWithRed: 1-144.0f/255.0f green: 1-159.0f/255.0f blue: 1-177.0f/255.0f alpha: 1.00] CGColor],
                       (id)[[UIColor colorWithRed: 1-183.0f/255.0f green: 1-193.0f/255.0f blue: 1-199.0f/255.0f  alpha: 1.00] CGColor], nil];
    else gradient.colors = [NSArray arrayWithObjects:(id)[[UIColor colorWithRed: 144.0f/255.0f green: 159.0f/255.0f blue: 177.0f/255.0f alpha: 1.00] CGColor],
                            (id)[[UIColor colorWithRed: 183.0f/255.0f green: 193.0f/255.0f blue: 199.0f/255.0f  alpha: 1.00] CGColor], nil];
    
    [customView.layer insertSublayer:gradient atIndex:0];
    
    CALayer *layerD = [CALayer layer];
    layerD.frame = CGRectMake(0.0, 23.0, tableView.bounds.size.width, 1.0);
    
    if (darkMode) layerD.backgroundColor = [[UIColor colorWithRed: 1-144.0f/255.0f green: 1-159.0f/255.0f blue: 1-177.0f/255.0f alpha: 1.00] CGColor];
    else layerD.backgroundColor = [[UIColor colorWithRed: 144.0f/255.0f green: 159.0f/255.0f blue: 177.0f/255.0f alpha: 1.00] CGColor];
    
    [customView.layer insertSublayer:layerD atIndex:0];
    
    UIButton *buttonLabel                  = [UIButton buttonWithType: UIButtonTypeCustom];
    buttonLabel.titleLabel.font            = [UIFont boldSystemFontOfSize: 20];
    buttonLabel.titleLabel.shadowOffset    = CGSizeMake (0.0, 1.0);
    buttonLabel.titleLabel.lineBreakMode   = (NSLineBreakMode)UILineBreakModeTailTruncation;
    //	buttonLabel.titleLabel.shadowOffset    = CGSizeMake (1.0, 0.0);
    buttonLabel.frame=CGRectMake(32, 0.0, tableView.bounds.size.width-32*2, 24);
    
    NSString *lbl;
    switch (section) {
        case 0:
            lbl= NSLocalizedString(@"Collections", @"");
            break;
        case 1:
            lbl= NSLocalizedString(@"Browse Internet", @"");
            break;
        case 2:
            lbl= NSLocalizedString(@"Games Music", @"");
            break;
        case 3:
            lbl= NSLocalizedString(@"Mods & chiptunes", @"");
            break;
        case 4:
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

/*
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    switch (section) {
        case 0:
            return NSLocalizedString(@"Collections", @"");
        case 1:
            return NSLocalizedString(@"Browse Internet", @"");
        case 2:
            return NSLocalizedString(@"Games Music", @"");
        case 3:
            return NSLocalizedString(@"Mods & chiptunes", @"");
        case 4:
            return NSLocalizedString(@"Other", @"");
        default:
            return nil;
    }
}*/

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return 5;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    switch (section) {
        case 0: //collections
            return ONLINE_COLLECTIONS_NUMBER;
        case 1: //internet
            return 2;
        case 2: //VGM
            return WEBLINKS_VGM_NB;
        case 3: //MODS
            return WEBLINKS_MODS_NB;
        case 4: //OThers
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
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    //NSLog(@"indexpath: %d %d %d/ %@ / %08X",indexPath.section,indexPath.row,cell_category,CellIdentifier,(void*)cell);
    if ((cell==nil)||forceReloadCells) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
        cell.frame=CGRectMake(0,0,tableView.frame.size.width,40);
        [cell setBackgroundColor:[UIColor clearColor]];
        
        NSString *imgFile=(darkMode?@"tabview_gradient40Black.png":@"tabview_gradient40.png");
        UIImage *image = [UIImage imageNamed:imgFile];
        
        UIImageView *imageView = [[UIImageView alloc] initWithImage:image];
        imageView.contentMode = UIViewContentModeScaleToFill;
        cell.backgroundView = imageView;
        //[imageView release];
        
        
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
        topLabel.font = [UIFont boldSystemFontOfSize:18];
        topLabel.lineBreakMode=NSLineBreakByTruncatingMiddle;
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
        bottomLabel.lineBreakMode=NSLineBreakByTruncatingMiddle;
        bottomLabel.opaque=TRUE;
        cell.accessoryView=nil;
        //cell.selectionStyle=UITableViewCellSelectionStyleGray;
    } else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
    }
    
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
                case ONLINE_COLLECTIONS_HVSC:topLabel.text=NSLocalizedString(@"HVSC collection",@"");
                    bottomLabel.text=[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),mNbHVSCFileEntries];
                    break;
                case ONLINE_COLLECTIONS_ASMA:topLabel.text=NSLocalizedString(@"ASMA collection",@"");
                    bottomLabel.text=[NSString stringWithFormat:NSLocalizedString(@"%d entries",@""),mNbASMAFileEntries];
                    break;
                case ONLINE_COLLECTIONS_JOSHW:topLabel.text=NSLocalizedString(@"JOSHW collection",@"");
                    bottomLabel.text=NSLocalizedString(@"Thousends of entries",@"");
                    break;
                case ONLINE_COLLECTIONS_VGMRips:topLabel.text=NSLocalizedString(@"VGMRips collection",@"");
                    bottomLabel.text=NSLocalizedString(@"2K+ packs / 40K+ songs",@"");
                    break;
                case ONLINE_COLLECTIONS_P2612:topLabel.text=NSLocalizedString(@"P2612 collection",@"");
                    bottomLabel.text=NSLocalizedString(@"700+ set / Sega Geneis/Megadrive",@"");
                    break;
                case ONLINE_COLLECTIONS_SNESM:topLabel.text=NSLocalizedString(@"SNESmusic collection",@"");
                    bottomLabel.text=NSLocalizedString(@"1500+ set / Super Nintendo/Famicom",@"");
                    break;
                case ONLINE_COLLECTIONS_SMSP:topLabel.text=NSLocalizedString(@"SMS Power! collection",@"");
                    bottomLabel.text=NSLocalizedString(@"Master System & Game gear sets",@"");
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
        case 2: //VGM
            topLabel.text=weblinks_VGM[indexPath.row][1];
            bottomLabel.text=weblinks_VGM[indexPath.row][0];
            break;
        case 3: //MODS & Chptunes
            topLabel.text=weblinks_MODS[indexPath.row][1];
            bottomLabel.text=weblinks_MODS[indexPath.row][0];
            break;
        case 4: //Others
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

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    }   
    else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }   
}
*/

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Navigation logic may go here. Create and push another view controller.
    /*
     <#DetailViewController#> *detailViewController = [[<#DetailViewController#> alloc] initWithNibName:@"<#Nib name#>" bundle:nil];
     // ...
     // Pass the selected object to the new view controller.
     [self.navigationController pushViewController:detailViewController animated:YES];
     [detailViewController release];
     */
    switch (indexPath.section) {
        case 0: {//collection
            switch (indexPath.row) {
                case ONLINE_COLLECTIONS_MODLAND: //MODLAND
                    collectionViewController = [[RootViewControllerMODLAND alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"MODLAND";
                    // Set new directory
                    ((RootViewControllerMODLAND*)collectionViewController)->browse_depth = 1;
                    ((RootViewControllerMODLAND*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerMODLAND*)collectionViewController)->downloadViewController=downloadViewController;
                    collectionViewController.view.frame=self.view.frame;
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_HVSC: //HVSC
                    collectionViewController =[[RootViewControllerHVSC alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"HVSC";
                    // Set new directory
                    ((RootViewControllerHVSC*)collectionViewController)->browse_depth = 1;
                    ((RootViewControllerHVSC*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerHVSC*)collectionViewController)->downloadViewController=downloadViewController;
                    collectionViewController.view.frame=self.view.frame;
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_ASMA: //ASMA
                    collectionViewController = [[RootViewControllerASMA alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"ASMA";
                    // Set new directory
                    ((RootViewControllerASMA*)collectionViewController)->browse_depth = 1;
                    ((RootViewControllerASMA*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerASMA*)collectionViewController)->downloadViewController=downloadViewController;
                    collectionViewController.view.frame=self.view.frame;
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_JOSHW: //JOSHW
                    collectionViewController = [[RootViewControllerJoshWWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"JoshW";
                    // Set new directory
                    ((RootViewControllerJoshWWebParser*)collectionViewController)->browse_depth = 0;
                    ((RootViewControllerJoshWWebParser*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerJoshWWebParser*)collectionViewController)->downloadViewController=downloadViewController;
                    
                    collectionViewController.view.frame=self.view.frame;
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_VGMRips: //VGMRips
                    collectionViewController = [[RootViewControllerVGMRWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"VGMRips";
                    // Set new directory
                    ((RootViewControllerVGMRWebParser*)collectionViewController)->browse_depth = 0;
                    ((RootViewControllerVGMRWebParser*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerVGMRWebParser*)collectionViewController)->downloadViewController=downloadViewController;
                    
                    collectionViewController.view.frame=self.view.frame;
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_P2612: //P2612
                    collectionViewController = [[RootViewControllerP2612WebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"P2612";
                    // Set new directory
                    ((RootViewControllerP2612WebParser*)collectionViewController)->browse_depth = 0;
                    ((RootViewControllerP2612WebParser*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerP2612WebParser*)collectionViewController)->downloadViewController=downloadViewController;
                    
                    collectionViewController.view.frame=self.view.frame;
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_SNESM: //SnesMusic
                    collectionViewController = [[RootViewControllerSNESMWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"SNESmusic";
                    // Set new directory
                    ((RootViewControllerSNESMWebParser*)collectionViewController)->browse_depth = 0;
                    ((RootViewControllerSNESMWebParser*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerSNESMWebParser*)collectionViewController)->downloadViewController=downloadViewController;
                    
                    collectionViewController.view.frame=self.view.frame;
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case ONLINE_COLLECTIONS_SMSP: //SMS Power!
                    collectionViewController = [[RootViewControllerSMSPWebParser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    //set new title
                    collectionViewController.title = @"SMS Power!";
                    // Set new directory
                    ((RootViewControllerSMSPWebParser*)collectionViewController)->browse_depth = 0;
                    ((RootViewControllerSMSPWebParser*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerSMSPWebParser*)collectionViewController)->downloadViewController=downloadViewController;
                    
                    collectionViewController.view.frame=self.view.frame;
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
            }
        }
            break;
        case 1:{//internet
                switch (indexPath.row) {
                    case 0: //World Charts
                        [webBrowser loadWorldCharts];
                        [self.navigationController pushViewController:webBrowser animated:YES];
                        break;
                    case 1: //Internet
                        [webBrowser loadLastURL];
                        [self.navigationController pushViewController:webBrowser animated:YES];
                        break;
                }
            }
            break;
        case 2: //VGM
            [webBrowser goToURLwithLoad:weblinks_VGM[indexPath.row][0]];
            [self.navigationController pushViewController:webBrowser animated:YES];
            break;
        case 3: //MODS & Chiptunes
            [webBrowser goToURL:weblinks_MODS[indexPath.row][0]];
            [self.navigationController pushViewController:webBrowser animated:YES];
            break;
        case 4: //Others
            [webBrowser goToURL:weblinks_Others[indexPath.row][0]];
            [self.navigationController pushViewController:webBrowser animated:YES];
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

@end
