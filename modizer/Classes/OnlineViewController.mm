//
//  OnlineViewController.m
//  modizer
//
//  Created by Yohann Magnien on 29/07/13.
//
//

//#define GET_NB_ENTRIES 1
#define NB_MODLAND_ENTRIES 319724
#define NB_HVSC_ENTRIES 43116


#import "OnlineViewController.h"

@interface OnlineViewController ()

@end

@implementation OnlineViewController

@synthesize tableView;
@synthesize downloadViewController,webBrowser,collectionViewController,detailViewController,playerButton;
@synthesize mNbMODLANDFileEntries,mNbHVSCFileEntries;


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
    
    self.tableView.rowHeight = 40;
    
#ifdef GET_NB_ENTRIES
    mNbMODLANDFileEntries=DBHelper::getNbMODLANDFilesEntries();
    mNbHVSCFileEntries=DBHelper::getNbHVSCFilesEntries();
#else
    mNbMODLANDFileEntries=NB_MODLAND_ENTRIES;
    mNbHVSCFileEntries=NB_HVSC_ENTRIES;
#endif


    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
 
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
    
//    [self.tableView registerClass:[UITableViewCell class] forCellReuseIdentifier:@"Cell"];
	//self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
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

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    return 5;
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
        CAGradientLayer *gradient = [CAGradientLayer layer];
        gradient.frame = cell.bounds;
        gradient.colors = [NSArray arrayWithObjects:
                           (id)[[UIColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:245.0/255.0 green:245.0/255.0 blue:245.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:240.0/255.0 green:240.0/255.0 blue:240.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:220.0/255.0 green:220.0/255.0 blue:220.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:220.0/255.0 green:220.0/255.0 blue:220.0/255.0 alpha:1] CGColor],
                           nil];
        gradient.locations = [NSArray arrayWithObjects:
                              (id)[NSNumber numberWithFloat:0.00f],
                              (id)[NSNumber numberWithFloat:0.03f],
                              (id)[NSNumber numberWithFloat:0.03f],
                              (id)[NSNumber numberWithFloat:0.97f],
                              (id)[NSNumber numberWithFloat:0.97f],
                              (id)[NSNumber numberWithFloat:1.00f],
                              nil];
        [cell setBackgroundView:[[UIView alloc] init]];
        [cell.backgroundView.layer insertSublayer:gradient atIndex:0];
        
        CAGradientLayer *selgrad = [CAGradientLayer layer];
        selgrad.frame = cell.bounds;
        selgrad.colors = [NSArray arrayWithObjects:
                          (id)[[UIColor colorWithRed:0.9f*220.0/255.0 green:0.99f*220.0/255.0 blue:0.9f*220.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:0.9f*220.0/255.0 green:0.99f*220.0/255.0 blue:0.9f*220.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:0.9f*240.0/255.0 green:0.99f*240.0/255.0 blue:0.9f*240.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:0.9f*245.0/255.0 green:0.99f*245.0/255.0 blue:0.9f*245.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:0.9f*255.0/255.0 green:0.99f*255.0/255.0 blue:0.9f*255.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:0.9f*255.0/255.0 green:0.99f*255.0/255.0 blue:0.9f*255.0/255.0 alpha:1] CGColor],
                          
                          nil];
        selgrad.locations = [NSArray arrayWithObjects:
                             (id)[NSNumber numberWithFloat:0.00f],
                             (id)[NSNumber numberWithFloat:0.03f],
                             (id)[NSNumber numberWithFloat:0.03f],
                             (id)[NSNumber numberWithFloat:0.97f],
                             (id)[NSNumber numberWithFloat:0.97f],
                             (id)[NSNumber numberWithFloat:1.00f],
                             nil];
        
        [cell setSelectedBackgroundView:[[UIView alloc] init]];
        [cell.selectedBackgroundView.layer insertSublayer:selgrad atIndex:0];
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
        topLabel.textColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:1.0 green:1.0 blue:0.9 alpha:1.0];
        topLabel.font = [UIFont boldSystemFontOfSize:20];
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
        bottomLabel.textColor = [UIColor colorWithRed:0.25 green:0.20 blue:0.20 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.75 green:0.8 blue:0.8 alpha:1.0];
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
    switch (indexPath.row) {
        case 0:topLabel.text=@"MODLAND collection";
            bottomLabel.text=[NSString stringWithFormat:@"%d entries",mNbMODLANDFileEntries];
            break;
        case 1:topLabel.text=@"HVSC collection";
            bottomLabel.text=[NSString stringWithFormat:@"%d entries",mNbHVSCFileEntries];
            break;
        case 2:topLabel.text=@"Modizer World Charts";
            bottomLabel.text=NSLocalizedString(@"Browser_WorldCharts_SubKey",@"");
            break;
        case 3:topLabel.text=@"Internet";
            bottomLabel.text=NSLocalizedString(@"Browser_Web_SubKey",@"");
            break;
        case 4:topLabel.text=@"Downloads";
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
    switch (indexPath.row) {
        case 0: //MODLAND
            collectionViewController = [[[RootViewControllerMODLAND alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]] autorelease];
            //set new title
            collectionViewController.title = @"MODLAND";
            // Set new directory
            ((RootViewControllerMODLAND*)collectionViewController)->browse_depth = 1;
            ((RootViewControllerMODLAND*)collectionViewController)->detailViewController=detailViewController;
            ((RootViewControllerMODLAND*)collectionViewController)->playerButton=playerButton;
            ((RootViewControllerMODLAND*)collectionViewController)->downloadViewController=downloadViewController;
            // And push the window
            [self.navigationController pushViewController:collectionViewController animated:YES];
            break;
        case 1: //HVSC
            collectionViewController = [[[RootViewControllerHVSC alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]] autorelease];
            //set new title
            collectionViewController.title = @"HVSC";
            // Set new directory
            ((RootViewControllerHVSC*)collectionViewController)->browse_depth = 1;
            ((RootViewControllerHVSC*)collectionViewController)->detailViewController=detailViewController;
            ((RootViewControllerHVSC*)collectionViewController)->playerButton=playerButton;
            ((RootViewControllerHVSC*)collectionViewController)->downloadViewController=downloadViewController;
            // And push the window
            [self.navigationController pushViewController:collectionViewController animated:YES];
            break;
        case 2: //World Charts            
            //didGoToWorldCharts=1;
            [webBrowser loadWorldCharts];
            [self.navigationController pushViewController:webBrowser animated:YES];
            break;
        case 3: //Internet
//            if (didGoToWorldCharts) {
//                didGoToWorldCharts=0;
                [webBrowser loadLastURL];
//            }
            [self.navigationController pushViewController:webBrowser animated:YES];
            break;
        case 4: //Downloads
            [self.navigationController pushViewController:downloadViewController animated:YES];
            break;
    }
}

@end
