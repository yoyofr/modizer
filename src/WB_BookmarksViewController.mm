//
//  WB_BookmarksViewController.m
//  modizer
//
//  Created by Yohann Magnien on 07/08/13.
//
//

#import "WB_BookmarksViewController.h"
#import "QuartzCore/CAGradientLayer.h"

/*
 */

@interface WB_BookmarksViewController ()

@end

@implementation WB_BookmarksViewController

@synthesize tableView,detailViewController,toolBar,webBrowser;
@synthesize waitingView,waitingViewPlayer;

#include "MiniPlayerImplementTableView.h"

-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:YES];
    else {
        UIAlertView *nofileplaying=[[UIAlertView alloc] initWithTitle:@"Warning"
                                                               message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
        [nofileplaying show];
    }
}

-(IBAction) closeBookmarks {
    [self saveBookmarks];
    //[self dismissViewControllerAnimated:YES completion:nil];
    [self.navigationController popToViewController:webBrowser animated:YES];
    //    [self release];
}
- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
    [super setEditing:editing animated:animated];
    [tableView setEditing:editing animated:animated];
    if (editing==FALSE) {
    } else {
        
    }
    [tableView reloadData];
}


- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

-(void) saveBookmarks {
	NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
            
	valNb=[[NSNumber alloc] initWithInt:custom_url_count];
	[prefs setObject:valNb forKey:@"Bookmarks_count"];
	for (int i=0;i<custom_url_count;i++) {
		[prefs setObject:custom_URL[i] forKey:[NSString stringWithFormat:@"Bookmark_URL%d",i]];
		[prefs setObject:custom_URL_name[i] forKey:[NSString stringWithFormat:@"Bookmark_URL_name%d",i]];
	}
	
    [prefs synchronize];
}

-(void) addbuiltinURL:(NSString *)URL name:(NSString *)name{
    builtin_URL[builtin_url_count]=URL;
    builtin_URL_name[builtin_url_count]=name;
    builtin_url_count++;
}
-(void) loadBuiltinBookmarks {
    builtin_url_count=0;
    /*[self addbuiltinURL:@"http://www.exotica.org.uk/" name:@"Exotica"];
    [self addbuiltinURL:@"http://amp.dascene.net/" name:@"Amiga Music Preservation"];
    [self addbuiltinURL:@"http://modarchive.org/" name:@"MOD Archive"];
    [self addbuiltinURL:@"http://sndh.atari.org/sndh/browser/index.php?dir=sndh_lf%2F" name:@"Atari ST SNDH Archive"];    
    [self addbuiltinURL:@"http://2a03.free.fr/" name:@"2A03"];
    [self addbuiltinURL:@"http://vgmrips.net/packs/" name:@"VGM Rips (Arcade, Computers, Consoles)"];
    [self addbuiltinURL:@"http://snesmusic.org/v2/" name:@"SNES Music"];
    [self addbuiltinURL:@"http://snesmusic.org/pmh/" name:@"Portable Music History"];
    [self addbuiltinURL:@"http://project2612.org/" name:@"Megadrive/Genesis Music"];
    
    [self addbuiltinURL:@"http://www.mirsoft.info/gamemods-archive.php" name:@"Mirsoft MODS"];
    [self addbuiltinURL:@"http://www.scene.org/dir.php?dir=/music" name:@"Scene.org"];
    [self addbuiltinURL:@"http://www.vgmusic.com/" name:@"VGMusic"];
    [self addbuiltinURL:@"http://www.mirsoft.info/gamemids-archive.php" name:@"Mirsoft Midis"];
    [self addbuiltinURL:@"http://www.midishrine.com/" name:@"Midishrine"];
    [self addbuiltinURL:@"http://www.lvbeethoven.com/Midi/index.html" name:@"Beethoven Midis"];
    */
}
-(void) loadBookmarks {
	NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    
    [prefs synchronize];
	
	valNb=[prefs objectForKey:@"Bookmarks_count"];
    if (valNb == nil) custom_url_count = 0;
    else custom_url_count = [valNb intValue];
    int custom_url_count_tmp=0;
    for (int i=0;i<custom_url_count;i++) {
        NSString *tmpstr1,*tmpstr2;
        tmpstr1=[prefs objectForKey:[NSString stringWithFormat:@"Bookmark_URL%d",i]];
        tmpstr2=[prefs objectForKey:[NSString stringWithFormat:@"Bookmark_URL_name%d",i]];
        if (tmpstr1 && tmpstr2) {
            custom_URL[custom_url_count_tmp]=[[NSString alloc] initWithString:tmpstr1];
            custom_URL_name[custom_url_count_tmp]=[[NSString alloc] initWithString:tmpstr2];
            custom_url_count_tmp++;
        }
    }
    custom_url_count=custom_url_count_tmp;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////

- (void)viewDidDisappear:(BOOL)animated {
    [repeatingTimer invalidate];
    repeatingTimer = nil;
    
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView removeObserver:self
                                          forKeyPath:observedSelector
                                             context:LoadingProgressObserverContext];
    [super viewDidDisappear:animated];
    
}


- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.navigationController.delegate = self;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    
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
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    waitingViewPlayer = [[WaitingView alloc] init];
    waitingViewPlayer.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingViewPlayer];
    
    views = NSDictionaryOfVariableBindings(waitingViewPlayer);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    
    
    [self loadBuiltinBookmarks];
    [self loadBookmarks];
    
    /*    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
     [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
     btn.adjustsImageWhenHighlighted = YES;
     [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
     UIBarButtonItem *item = [[[UIBarButtonItem alloc] initWithCustomView: btn] autorelease];
     self.navigationItem.rightBarButtonItem = item;*/
    
    //    UIBarButtonItem *btnBar=[toolBar.items objectAtIndex:0];
    if( list_builtin==0) {
        NSMutableArray *toolBarItems=[[NSMutableArray alloc] init];
        [toolBarItems addObject:self.editButtonItem];
        [toolBar setItems:toolBarItems animated:NO];
    }
    
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    
	valNb=[[NSNumber alloc] initWithInt:custom_url_count];
	[prefs setObject:valNb forKey:@"Bookmarks_count"];
	for (int i=0;i<custom_url_count;i++) {
		[prefs setObject:custom_URL[i] forKey:[NSString stringWithFormat:@"Bookmark_URL%d",i]];
		[prefs setObject:custom_URL_name[i] forKey:[NSString stringWithFormat:@"Bookmark_URL_name%d",i]];
	}
    
    [prefs synchronize];
    
    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
    
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
}

- (void)viewDidAppear:(BOOL)animated {
    [repeatingTimer invalidate];
    repeatingTimer = nil;
    
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView removeObserver:self
                                          forKeyPath:observedSelector
                                             context:LoadingProgressObserverContext];
    
    [super viewDidAppear:animated];
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}


- (void)viewWillAppear:(BOOL)animated {
    self.navigationController.delegate = self;
    [super viewWillAppear:animated];
    
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    
    self.navigationController.delegate = self;
    
    if ([detailViewController mPlaylist_size]>0) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    } else {
        wasMiniPlayerOn=false;
        [self hideMiniPlayer];
    }
    
    [self hideWaiting];
    
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
 
}

- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
    //    return UIInterfaceOrientationMaskAllButUpsideDown;
}

- (BOOL)shouldAutorotate {
    [self shouldAutorotateToInterfaceOrientation:self.interfaceOrientation];
    return TRUE;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	[self.tableView reloadData];
	return YES;
}

- (void)dealloc {
	[self saveBookmarks];
	for (int i=0;i<custom_url_count;i++) {
        custom_URL[i]=nil;//[custom_URL[i] release];
        custom_URL_name[i]=nil;//[custom_URL_name[i] release];
	}
    
    [waitingView removeFromSuperview];
    [waitingViewPlayer removeFromSuperview];
    waitingView=nil;
    waitingViewPlayer=nil;
    //[super dealloc];
}


- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
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


#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    if (list_builtin) return builtin_url_count;
    return custom_url_count;//+1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    NSString *cellValue;
    const NSInteger TOP_LABEL_TAG = 1001;
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    UILabel *topLabel;
    UILabel *bottomLabel;
    BOOL isEditing=[tableView isEditing];
    
    if (forceReloadCells) {
        while ([tableView dequeueReusableCellWithIdentifier:CellIdentifier]) {}
        forceReloadCells=false;
    }
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        //        cell = [[[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:CellIdentifier] autorelease];
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
        cell.frame=CGRectMake(0,0,tableView.frame.size.width,40);
        
        [cell setBackgroundColor:[UIColor clearColor]];
        
        /*CAGradientLayer *gradient = [CAGradientLayer layer];
        gradient.frame = CGRectMake(0,0,tableView.frame.size.width*2,40);
        gradient.colors = [NSArray arrayWithObjects:
                           (id)[[UIColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:235.0/255.0 green:235.0/255.0 blue:235.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:240.0/255.0 green:240.0/255.0 blue:240.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:200.0/255.0 green:200.0/255.0 blue:200.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:200.0/255.0 green:200.0/255.0 blue:200.0/255.0 alpha:1] CGColor],
                           nil];
        gradient.locations = [NSArray arrayWithObjects:
                              (id)[NSNumber numberWithFloat:0.00f],
                              (id)[NSNumber numberWithFloat:0.03f],
                              (id)[NSNumber numberWithFloat:0.03f],
                              (id)[NSNumber numberWithFloat:0.97f],
                              (id)[NSNumber numberWithFloat:0.97f],
                              (id)[NSNumber numberWithFloat:1.00f],
                              nil];
        UIView *bgview=[[UIView alloc] init];
        bgview.autoresizingMask=UIViewAutoresizingFlexibleWidth;
        [cell setBackgroundView:bgview];
        [cell.backgroundView.layer insertSublayer:gradient atIndex:0];
        
        CAGradientLayer *selgrad = [CAGradientLayer layer];
        selgrad.frame = CGRectMake(0,0,tableView.frame.size.width*2,40);
        float rev_col_adj=1.2f;
        selgrad.colors = [NSArray arrayWithObjects:
                          (id)[[UIColor colorWithRed:rev_col_adj-255.0/255.0 green:rev_col_adj-255.0/255.0 blue:rev_col_adj-255.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:rev_col_adj-255.0/255.0 green:rev_col_adj-255.0/255.0 blue:rev_col_adj-255.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:rev_col_adj-235.0/255.0 green:rev_col_adj-235.0/255.0 blue:rev_col_adj-235.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:rev_col_adj-240.0/255.0 green:rev_col_adj-240.0/255.0 blue:rev_col_adj-240.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:rev_col_adj-200.0/255.0 green:rev_col_adj-200.0/255.0 blue:rev_col_adj-200.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:rev_col_adj-200.0/255.0 green:rev_col_adj-200.0/255.0 blue:rev_col_adj-200.0/255.0 alpha:1] CGColor],
                          nil];
        selgrad.locations = [NSArray arrayWithObjects:
                             (id)[NSNumber numberWithFloat:0.00f],
                             (id)[NSNumber numberWithFloat:0.03f],
                             (id)[NSNumber numberWithFloat:0.03f],
                             (id)[NSNumber numberWithFloat:0.97f],
                             (id)[NSNumber numberWithFloat:0.97f],
                             (id)[NSNumber numberWithFloat:1.00f],
                             nil];
        
        
        
        bgview=[[UIView alloc] init];
        bgview.autoresizingMask=UIViewAutoresizingFlexibleWidth;
        [cell setSelectedBackgroundView:bgview];
        [cell.selectedBackgroundView.layer insertSublayer:selgrad atIndex:0];
         */
        
        NSString *imgFilename=(darkMode?@"tabview_gradient40Black.png":@"tabview_gradient40.png");
        UIImage *image = [UIImage imageNamed:imgFilename];
        UIImageView *imageView = [[UIImageView alloc] initWithImage:image];
        imageView.contentMode = UIViewContentModeScaleToFill;
        cell.backgroundView = imageView;
        
        
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
        //        cell.selectionStyle=UITableViewCellSelectionStyleGray;
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
                               tableView.bounds.size.width -1.0 * cell.indentationWidth- 32-(isEditing?32:0),
                               22);
    bottomLabel.frame = CGRectMake(1.0 * cell.indentationWidth,
                                   22,
                                   tableView.bounds.size.width -1.0 * cell.indentationWidth-32-(isEditing?32:0),
                                   18);
    
    bottomLabel.text=@""; //default value
    
    cell.accessoryType = UITableViewCellAccessoryNone;
    
    if (list_builtin) {
        topLabel.text=builtin_URL_name[indexPath.row];
        bottomLabel.text=builtin_URL[indexPath.row];
    } else {
        /*if (indexPath.row==0) {
            topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
            topLabel.text=NSLocalizedString(@"Builtin URLs", @"");
            bottomLabel.text=NSLocalizedString(@"Mods, spc, vgm, midis, ...", @"");
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        } else {*/
            topLabel.text=custom_URL_name[indexPath.row];//-1];
            bottomLabel.text=custom_URL[indexPath.row];//-1];
        //}
    }
    
    return cell;
}

// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    if (list_builtin) return NO;
    //if (indexPath.row==0) return NO;
    return YES;
}

// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        
        //for (int i=indexPath.row-1;i<custom_url_count-1;i++) {
         for (int i=indexPath.row;i<custom_url_count-1;i++) {
            custom_URL_name[i]=custom_URL_name[i+1];
            custom_URL[i]=custom_URL[i+1];
        }
        custom_URL_name[custom_url_count-1]=nil;
        custom_URL[custom_url_count-1]=nil;
        custom_url_count--;
        
        [self saveBookmarks];
        
//        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
        [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
        
    }
    else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}

// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
    /*NSString *tmpStr=custom_URL_name[fromIndexPath.row-1];
    custom_URL_name[fromIndexPath.row-1]=custom_URL_name[toIndexPath.row-1];
    custom_URL_name[toIndexPath.row-1]=tmpStr;
    
    tmpStr=custom_URL[fromIndexPath.row-1];
    custom_URL[fromIndexPath.row-1]=custom_URL[toIndexPath.row-1];
    custom_URL[toIndexPath.row-1]=tmpStr;*/
    
    NSString *tmpStr=custom_URL_name[fromIndexPath.row];
    custom_URL_name[fromIndexPath.row]=custom_URL_name[toIndexPath.row];
    custom_URL_name[toIndexPath.row]=tmpStr;
    
    tmpStr=custom_URL[fromIndexPath.row];
    custom_URL[fromIndexPath.row]=custom_URL[toIndexPath.row];
    custom_URL[toIndexPath.row]=tmpStr;
    
    [self saveBookmarks];
}

// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    if (list_builtin) return NO;
    return YES;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    /*if (list_builtin) {
        [webBrowser goToURL:builtin_URL[indexPath.row]];
        [self closeBookmarks];
    } else {*/
        /*if (indexPath.row==0) {
            //display builtin
            WB_BookmarksViewController *bookmarksVC = [[[WB_BookmarksViewController alloc]  initWithNibName:@"BookmarksViewController" bundle:[NSBundle mainBundle]] autorelease];
            //set new title
            bookmarksVC.title = NSLocalizedString(@"Builtin Bookmarks",@"");
            bookmarksVC->detailViewController = detailViewController;
            bookmarksVC->webBrowser=webBrowser;
            bookmarksVC->list_builtin=1;
            [self.navigationController pushViewController:bookmarksVC animated:YES];
        }
        if (indexPath.row>=1) {*/
        [webBrowser goToURL:custom_URL[indexPath.row]];//-1]];
        [self closeBookmarks];
      //  }
    //}
}

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    }
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
