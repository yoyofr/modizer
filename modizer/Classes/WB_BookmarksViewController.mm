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

-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:(detailViewController.mSlowDevice?NO:YES)];
    else {
        UIAlertView *nofileplaying=[[[UIAlertView alloc] initWithTitle:@"Warning"
                                                               message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
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
	[prefs setObject:valNb forKey:@"Bookmarks_count"];[valNb autorelease];
	for (int i=0;i<custom_url_count;i++) {
		[prefs setObject:custom_URL[i] forKey:[NSString stringWithFormat:@"Bookmark_URL%d",i]];
		[prefs setObject:custom_URL_name[i] forKey:[NSString stringWithFormat:@"Bookmark_URL_name%d",i]];
	}
	
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


- (void)viewDidLoad
{
    [super viewDidLoad];
    
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
        NSMutableArray *toolBarItems=[[[NSMutableArray alloc] init] autorelease];
        [toolBarItems addObject:self.editButtonItem];
        [toolBar setItems:toolBarItems animated:NO];
    }
    
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    
	valNb=[[NSNumber alloc] initWithInt:custom_url_count];
	[prefs setObject:valNb forKey:@"Bookmarks_count"];[valNb autorelease];
	for (int i=0;i<custom_url_count;i++) {
		[prefs setObject:custom_URL[i] forKey:[NSString stringWithFormat:@"Bookmark_URL%d",i]];
		[prefs setObject:custom_URL_name[i] forKey:[NSString stringWithFormat:@"Bookmark_URL_name%d",i]];
	}
    
    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
    
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
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
		[custom_URL[i] release];
		[custom_URL_name[i] release];
	}
    [super dealloc];
}


- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
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
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        //        cell = [[[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:CellIdentifier] autorelease];
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
        
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
        UIImage *image = [UIImage imageNamed:@"tabview_gradient40.png"];
        UIImageView *imageView = [[UIImageView alloc] initWithImage:image];
        imageView.contentMode = UIViewContentModeScaleToFill;
        cell.backgroundView = imageView;
        [imageView release];
        
        //
        // Create the label for the top row of text
        //
        topLabel = [[[UILabel alloc] init] autorelease];
        [cell.contentView addSubview:topLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        topLabel.tag = TOP_LABEL_TAG;
        topLabel.backgroundColor = [UIColor clearColor];
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        topLabel.font = [UIFont boldSystemFontOfSize:18];
        topLabel.lineBreakMode=NSLineBreakByTruncatingMiddle;
        topLabel.opaque=TRUE;
        
        //
        // Create the label for the top row of text
        //
        bottomLabel = [[[UILabel alloc] init] autorelease];
        [cell.contentView addSubview:bottomLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        bottomLabel.tag = BOTTOM_LABEL_TAG;
        bottomLabel.backgroundColor = [UIColor clearColor];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
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
    topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
    topLabel.highlightedTextColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
    bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
    bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
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

@end
