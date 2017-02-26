//
//  SettingsMaintenanceViewController.m
//  modizer
//
//  Created by Yohann Magnien on 10/08/13.
//
//

#import "SettingsMaintenanceViewController.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;

extern BOOL is_ios7;

@interface SettingsMaintenanceViewController ()
@end

@implementation SettingsMaintenanceViewController

@synthesize tableView,detailViewController,rootVC;

-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:(detailViewController.mSlowDevice?NO:YES)];
    else {
        UIAlertView *nofileplaying=[[[UIAlertView alloc] initWithTitle:@"Warning"
                                                               message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
        [nofileplaying show];
    }
}

-(void) shortWait {
    [NSThread sleepForTimeInterval:0.1f];
}

-(void)showWaiting{
	waitingView.hidden=FALSE;
}

-(void)hideWaiting{
	waitingView.hidden=TRUE;
}

- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex {
    [self hideWaiting];
}


- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}


- (void)viewDidLoad
{
    [super viewDidLoad];
    
    UIButton *btn = [[[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)] autorelease];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[[UIBarButtonItem alloc] initWithCustomView: btn] autorelease];
    self.navigationItem.rightBarButtonItem = item;
    
    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
    
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
    
    /////////////////////////////////////
    // Waiting view
    /////////////////////////////////////
    waitingView = [[UIView alloc] init];
    waitingView.backgroundColor=[UIColor blackColor];//[UIColor colorWithRed:0 green:0 blue:0 alpha:0.8f];
    waitingView.opaque=YES;
    waitingView.hidden=TRUE;
    waitingView.layer.cornerRadius=20;
    
    UIActivityIndicatorView *indView=[[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(50-20,50-20,40,40)];
    indView.activityIndicatorViewStyle=UIActivityIndicatorViewStyleWhiteLarge;
    [waitingView addSubview:indView];
    
    [indView startAnimating];
    [indView autorelease];
    
    waitingView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:waitingView];
    
    NSDictionary *views = NSDictionaryOfVariableBindings(waitingView);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingView(100)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingView(100)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    /////////////////////////////////////////

}

- (void)viewWillAppear:(BOOL)animated {
    if (!is_ios7) {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleBlack];
    } else {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    }
    [super viewWillAppear:animated];
}

-(void) viewDidAppear:(BOOL)animated {
//    [self.tableView reloadData];
    [super viewDidAppear:animated];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void) resetSettings {
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
    [SettingsGenViewController applyDefaultSettings];
    UIAlertView *alert = [[[UIAlertView alloc] initWithTitle: @"Info" message:NSLocalizedString(@"Settings reseted",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
    [alert show];
    
}

-(bool) resetRatingsDB {
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[256];
		
		sprintf(sqlStatement,"UPDATE user_stats SET rating=NULL");
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
	};
	sqlite3_close(db);

    UIAlertView *alert = [[[UIAlertView alloc] initWithTitle: @"Info" message:NSLocalizedString(@"Ratings reseted",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
    [alert show];
    
	return TRUE;
}

-(bool) resetPlaycountDB {
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[256];
		
		sprintf(sqlStatement,"UPDATE user_stats SET play_count=0");
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
	};
	sqlite3_close(db);
    
    UIAlertView *alert = [[[UIAlertView alloc] initWithTitle: @"Info" message:NSLocalizedString(@"Played Counters reseted",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
    [alert show];
    
	return TRUE;
}

-(bool) cleanDB {
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	BOOL success;
	NSFileManager *fileManager = [[NSFileManager alloc] init];
    
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[256];
		char sqlStatement2[256];
		sqlite3_stmt *stmt;
		
		
		//First check that user_stats entries still exist
		sprintf(sqlStatement,"SELECT fullpath FROM user_stats");
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				success = [fileManager fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"%s",sqlite3_column_text(stmt, 0)]]];
				if (!success) {//file does not exist
					//NSLog(@"missing : %s",sqlite3_column_text(stmt, 0));
					
					sprintf(sqlStatement2,"DELETE FROM user_stats WHERE fullpath=\"%s\"",sqlite3_column_text(stmt, 0));
					err=sqlite3_exec(db, sqlStatement2, NULL, NULL, NULL);
					if (err!=SQLITE_OK) {
						NSLog(@"Issue during delete of user_Stats");
					}
				}
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		//Second check that playlist entries still exist
		sprintf(sqlStatement,"SELECT fullpath FROM playlists_entries");
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				success = [fileManager fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"%s",sqlite3_column_text(stmt, 0)]]];
				if (!success) {//file does not exist
					NSLog(@"missing : %s",sqlite3_column_text(stmt, 0));
					
					sprintf(sqlStatement2,"DELETE FROM playlists_entries WHERE fullpath=\"%s\"",sqlite3_column_text(stmt, 0));
					err=sqlite3_exec(db, sqlStatement2, NULL, NULL, NULL);
					if (err!=SQLITE_OK) {
						NSLog(@"Issue during delete of playlists_entries");
					}
				}
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		//No defrag DB
		sprintf(sqlStatement2,"VACUUM");
		err=sqlite3_exec(db, sqlStatement2, NULL, NULL, NULL);
		if (err!=SQLITE_OK) {
			NSLog(@"Issue during VACUUM");
		}
	};
	sqlite3_close(db);
	
	
	pthread_mutex_unlock(&db_mutex);
    [fileManager release];
	
    UIAlertView *alert = [[[UIAlertView alloc] initWithTitle: @"Info" message:NSLocalizedString(@"Database cleaned",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
    [alert show];
    
	return TRUE;
}

-(void) recreateSamplesFolder {
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
    
    [rootVC createSamplesFromPackage:TRUE];
    UIAlertView *alert = [[[UIAlertView alloc] initWithTitle: @"Info" message:NSLocalizedString(@"Samples folder created",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
    [alert show];
}

-(void) resetDB {
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
    [rootVC createEditableCopyOfDatabaseIfNeeded:TRUE quiet:TRUE];
    UIAlertView *alert = [[[UIAlertView alloc] initWithTitle: @"Info" message:NSLocalizedString(@"Database reseted",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
    [alert show];
}

-(void) removeCurrentCover {
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
    NSError *err;
    NSFileManager *mFileMngr=[[NSFileManager alloc] init];
    NSString *currentPlayFilepath =[detailViewController getCurrentModuleFilepath];
    if (currentPlayFilepath==nil) {
        UIAlertView *alert = [[[UIAlertView alloc] initWithTitle: @"Info" message:NSLocalizedString(@"No cover to remove",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
        [alert show];
        return;
    }
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.jpg",NSHomeDirectory(),[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.png",NSHomeDirectory(),[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.gif",NSHomeDirectory(),[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.jpg",NSHomeDirectory(),[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.png",NSHomeDirectory(),[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
    [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.gif",NSHomeDirectory(),[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
    [mFileMngr release];
    
    UIAlertView *alert = [[[UIAlertView alloc] initWithTitle: @"Info" message:NSLocalizedString(@"Cover removed",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
    [alert show];
}


#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return 7;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    NSString *title=nil;
    return title;
}




- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    NSString *footer=nil;
    return footer;
}


- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    NSString *cellValue;
    const NSInteger TOP_LABEL_TAG = 1001;
    UILabel *topLabel;
    BButton *btn;
    
    UITableViewCell *cell = [tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
        
        cell.frame=CGRectMake(0,0,self.view.frame.size.width,50);
        cell.autoresizingMask=UIViewAutoresizingFlexibleWidth;
        [cell setBackgroundColor:[UIColor clearColor]];
        
        UIImage *image = [UIImage imageNamed:@"tabview_gradient50.png"];
        UIImageView *imageView = [[UIImageView alloc] initWithImage:image];
        imageView.contentMode = UIViewContentModeScaleToFill;
        cell.backgroundView = imageView;
        [imageView release];
        
        
        btn= [[[BButton alloc] initWithFrame:CGRectMake(self.view.frame.size.width/2-100,
                                                      10,
                                                      200,
          
                                                       30)] autorelease];
        btn.tag=TOP_LABEL_TAG;
        [cell.contentView addSubview:btn];
        btn.autoresizingMask=UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleRightMargin;
        
        cell.accessoryView=nil;
        cell.accessoryType = UITableViewCellAccessoryNone;
        [cell layoutSubviews];
    } else {
        btn = (BButton *)[cell viewWithTag:TOP_LABEL_TAG];
    }    
    btn.frame=CGRectMake(self.view.frame.size.width/2-100,10,200,30);
    
    
    NSString *txt;
    switch (indexPath.row) {            
        case 0: //Clean DB
            txt=NSLocalizedString(@"Clean Database",@"");
            [btn setType:BButtonTypePrimary];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(cleanDB) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 1: //Recreate Samples folder
            txt=NSLocalizedString(@"Recreate Samples folder",@"");
            [btn setType:BButtonTypePrimary];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(recreateSamplesFolder) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 2: //Reset settings to default
            txt=NSLocalizedString(@"Reset settings to default",@"");
            [btn setType:BButtonTypeWarning];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetSettings) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 3: //Remove current cover
            txt=NSLocalizedString(@"Remove current cover",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(removeCurrentCover) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 4: //Reset Ratings
            txt=NSLocalizedString(@"Reset Ratings",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetRatingsDB) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 5: //Reset played counter
            txt=NSLocalizedString(@"Reset Played Counters",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetPlaycountDB) forControlEvents:UIControlEventTouchUpInside];
            break;
        case 6: //Reset DB
            txt=NSLocalizedString(@"Reset Database",@"");
            [btn setType:BButtonTypeDanger];
            [btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
            [btn addTarget:self action:@selector(resetDB) forControlEvents:UIControlEventTouchUpInside];
            break;

    }
    [btn setTitle:txt forState:UIControlStateNormal];
    
    
    return cell;
}

/*
 // Override to support conditional editing of the table view.
 - (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath
 {
 // Return NO if you do not want the specified item to be editable.
 return YES;
 }
 */

/*
 // Override to support editing the table view.
 - (void)tableView:(UITableView *)tabView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
 {
 if (editingStyle == UITableViewCellEditingStyleDelete) {
 // Delete the row from the data source
 [tabView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
 }
 else if (editingStyle == UITableViewCellEditingStyleInsert) {
 // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
 }
 }
 */

/*
 // Override to support rearranging the table view.
 - (void)tableView:(UITableView *)tabView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
 {
 }
 */

/*
 // Override to support conditional rearranging of the table view.
 - (BOOL)tableView:(UITableView *)tabView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
 {
 // Return NO if you do not want the item to be re-orderable.
 return YES;
 }
 */

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tabView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
}



@end
