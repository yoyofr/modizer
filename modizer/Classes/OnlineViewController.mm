//
//  OnlineViewController.m
//  modizer
//
//  Created by Yohann Magnien on 29/07/13.
//
//

extern BOOL is_ios7;

#define GET_NB_ENTRIES 1
//#define NB_MODLAND_ENTRIES 319746
//#define NB_HVSC_ENTRIES 43856
//#define NB_ASMA_ENTRIES 4630


#import "OnlineViewController.h"

@interface OnlineViewController ()

@end

@implementation OnlineViewController

@synthesize tableView;
@synthesize downloadViewController,webBrowser,collectionViewController,detailViewController;
@synthesize mNbMODLANDFileEntries,mNbHVSCFileEntries,mNbASMAFileEntries;


- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
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
    UIBarButtonItem *item = [[[UIBarButtonItem alloc] initWithCustomView: btn] autorelease];
    self.navigationItem.rightBarButtonItem = item;
    
    
//    [self.tableView registerClass:[UITableViewCell class] forCellReuseIdentifier:@"Cell"];
	//self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
}

-(void) viewWillAppear:(BOOL)animated {
    if (!is_ios7) {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleBlack];
    } else {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    }
    
    
    [self.tableView reloadData];
    collectionViewController=nil;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(IBAction) goPlayer {
    [self.navigationController pushViewController:detailViewController animated:(detailViewController.mSlowDevice?NO:YES)];

}

#pragma mark - Table view data source

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    switch (section) {
        case 0:
            return NSLocalizedString(@"Collections", @"");
        case 1:
            return NSLocalizedString(@"Internet", @"");
        default:
            return nil;
    }
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    switch (section) {
        case 0: //collections
            return 3;
        case 1: //internet
            return 2;
        default:
            return 0;
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    const NSInteger TOP_LABEL_TAG = 1001;
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    UILabel *topLabel;
    UILabel *bottomLabel;
    
    static NSString *CellIdentifier = @"Cell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    if (cell==nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
        
        cell.frame=CGRectMake(0,0,tableView.frame.size.width,40);
        [cell setBackgroundColor:[UIColor clearColor]];
        
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
        topLabel.lineBreakMode=UILineBreakModeMiddleTruncation;
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
        bottomLabel.lineBreakMode=UILineBreakModeMiddleTruncation;
        bottomLabel.opaque=TRUE;
        cell.accessoryView=nil;
        //cell.selectionStyle=UITableViewCellSelectionStyleGray;
    } else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
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
                case 0:topLabel.text=NSLocalizedString(@"MODLAND collection",@"");
                    bottomLabel.text=[NSString stringWithFormat:@"%d entries",mNbMODLANDFileEntries];
                    break;
                case 1:topLabel.text=NSLocalizedString(@"HVSC collection",@"");
                    bottomLabel.text=[NSString stringWithFormat:@"%d entries",mNbHVSCFileEntries];
                    break;
                case 2:topLabel.text=NSLocalizedString(@"ASMA collection",@"");
                    bottomLabel.text=[NSString stringWithFormat:@"%d entries",mNbASMAFileEntries];
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
                    break;
            }
        }
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
                case 0: //MODLAND
                    collectionViewController = [[[RootViewControllerMODLAND alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]] autorelease];
                    //set new title
                    collectionViewController.title = @"MODLAND";
                    // Set new directory
                    ((RootViewControllerMODLAND*)collectionViewController)->browse_depth = 1;
                    ((RootViewControllerMODLAND*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerMODLAND*)collectionViewController)->downloadViewController=downloadViewController;
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case 1: //HVSC
                    collectionViewController = [[[RootViewControllerHVSC alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]] autorelease];
                    //set new title
                    collectionViewController.title = @"HVSC";
                    // Set new directory
                    ((RootViewControllerHVSC*)collectionViewController)->browse_depth = 1;
                    ((RootViewControllerHVSC*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerHVSC*)collectionViewController)->downloadViewController=downloadViewController;
                    // And push the window
                    [self.navigationController pushViewController:collectionViewController animated:YES];
                    break;
                case 2: //ASMA
                    collectionViewController = [[[RootViewControllerASMA alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]] autorelease];
                    //set new title
                    collectionViewController.title = @"ASMA";
                    // Set new directory
                    ((RootViewControllerASMA*)collectionViewController)->browse_depth = 1;
                    ((RootViewControllerASMA*)collectionViewController)->detailViewController=detailViewController;
                    ((RootViewControllerASMA*)collectionViewController)->downloadViewController=downloadViewController;
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
    }
}

-(void) refreshViewAfterDownload {
    [tableView reloadData];
    if (collectionViewController) [collectionViewController refreshViewAfterDownload];
}

@end
