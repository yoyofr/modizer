//
//  SettingsViewController.m
//  modizer4
//
//  Created by yoyofr on 7/23/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//
#include <pthread.h>
extern pthread_mutex_t db_mutex;

#import <CFNetwork/CFNetwork.h>
#import "SettingsViewController.h"
#import <QuartzCore/CAGradientLayer.h>
#import "Reachability.h"

#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/xattr.h>

static NSString *currentPlayFilepath=nil;

@implementation SettingsViewController

@synthesize detailViewControllerIphone,rootViewControllerIphone;
@synthesize scrollSettingsView,settingsView;
@synthesize cSettingsViewADPLUG,cSettingsViewSEXYPSF,cSettingsViewAOSDK;
@synthesize cSettingsViewGENPLAYER,cSettingsViewGENDISPLAY,cSettingsViewDOWNLOAD;
@synthesize cSettingsViewDUMB,cSettingsViewMODPLUG,cSettingsViewUADE,cSettingsViewGME,cSettingsViewFTP;
@synthesize cSettingsViewSID,cSettingsViewTIM;
@synthesize lbl_FTPstatus,sc_FTPswitch,sc_FTPanonymous;
@synthesize ftpUser,ftpPassword,ftpPort;



#pragma mark -
#pragma mark Initialization
/*
 - (BOOL)textFieldShouldEndEditing:(UITextField *)textField {
 [textField resignFirstResponder];
 return NO;
 }
 
 - (void)textFieldDidEndEditing:(UITextField *)textField {
 [textField resignFirstResponder];
 
 }*/

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return NO;
}

-(IBAction) FTPanonChanged {
}

- (BOOL)addSkipBackupAttributeToItemAtPath:(NSString*)path
{
    //    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    //  NSString *documentsDirectory = [paths objectAtIndex:0];    
    const char* filePath = [path fileSystemRepresentation];
    
    const char* attrName = "com.apple.MobileBackup";
    u_int8_t attrValue = 1;
    
    int result = setxattr(filePath, attrName, &attrValue, sizeof(attrValue), 0, 0);
    return result == 0;
}

-(void) updateFilesDoNotBackupAttributes {
    NSError *error;
    NSArray *dirContent;
    int result;
    //BOOL isDir;
    NSFileManager *mFileMngr = [[NSFileManager alloc] init];
    NSString *cpath=[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/"];
    NSString *file;
    const char* attrName = "com.apple.MobileBackup";
    u_int8_t attrValue = 1;
    
    dirContent=[mFileMngr subpathsOfDirectoryAtPath:cpath error:&error];
    for (file in dirContent) {
        //NSLog(@"%@",file);
//        [mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
        result = setxattr([[cpath stringByAppendingFormat:@"/%@",file] fileSystemRepresentation], attrName, &attrValue, sizeof(attrValue), 0, 0);
        if (result) NSLog(@"Issue %d when settings nobackup flag on %@",result,[cpath stringByAppendingFormat:@"/%@",file]);
    }
    [mFileMngr release];
}


- (NSString *)getIPAddress {
	NSString *address = @"error";
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *temp_addr = NULL;
	int success = 0;
	
	// retrieve the current interfaces - returns 0 on success
	success = getifaddrs(&interfaces);
	if (success == 0)
	{
		// Loop through linked list of interfaces
		temp_addr = interfaces;
		while(temp_addr != NULL)
		{
			if(temp_addr->ifa_addr->sa_family == AF_INET)
			{
				// Check if interface is en0 which is the wifi connection on the iPhone
				if([[NSString stringWithUTF8String:temp_addr->ifa_name] isEqualToString:@"en0"])
				{
					// Get NSString from C String
					address = [NSString stringWithUTF8String:inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr)];
				}
			}
			
			temp_addr = temp_addr->ifa_next;
		}
	}
	
	// Free memory
	freeifaddrs(interfaces);
	
	return address;
}


-(IBAction) FTPswitchChanged {
	if (sc_FTPswitch.selectedSegmentIndex) {
        //		NSLog(@"reach status : %d",[[Reachability reachabilityForLocalWiFi] currentReachabilityStatus]);
		if ([[Reachability reachabilityForLocalWiFi] currentReachabilityStatus]==kReachableViaWiFi) {
			if (!bServerRunning) { // Start the FTP Server
				if ([self startFTPServer]) {
					bServerRunning = true;
					//NSHost *currentHost = [NSHost currentHost]; 
					
					NSString *ip = [self getIPAddress];//@"";
					/*NSString *tmpStr;
                     for (int i=0;i<[[currentHost addresses] count];i++) {
                     tmpStr=[[currentHost addresses] objectAtIndex:i];
                     if (!strchr([tmpStr UTF8String],':')) {
                     if (!strstr([tmpStr UTF8String],"127.0.0.1")) {
                     ip = [NSString stringWithFormat:@"%@",tmpStr ];
                     NSLog(@"found : %@",ip); //assumption : last address will be WIFI one
                     //break;
                     }
                     }
                     }*/					
					NSString *msg = [NSString stringWithFormat:@"Listening on %@\nPort : %@", ip,ftpPort.text];
					lbl_FTPstatus.text = msg;

                    // Disable idle timer to avoid wifi connection lost
                    [UIApplication sharedApplication].idleTimerDisabled=YES;
				} else {
					bServerRunning = false;
					UIAlertView *alert = [[[UIAlertView alloc] initWithTitle: @"Error" message:@"Warning: Unable to start FTP Server." delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease];
					[alert show];
					sc_FTPswitch.selectedSegmentIndex=0;
				}
			}
			
		} else {
			UIAlertView *alert = [[[UIAlertView alloc] initWithTitle: @"Warning" message:@"FTP server can only run on a WIFI connection." delegate:nil cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
			[alert show];
			sc_FTPswitch.selectedSegmentIndex=0;
		}
	} else {
		if (bServerRunning) { // Stop FTP server
			// Stop the server
			ftpserver->StopListening();
			// Delete users
			ftpserver->DeleteUser(pAnonymousUser);
			ftpserver->DeleteUser(pUser);
			bServerRunning = false;
			lbl_FTPstatus.text = @"Server is stopped.";
            
            // Restart idle timer if battery mode is on (unplugged device)
            if ([[UIDevice currentDevice] batteryState] != UIDeviceBatteryStateUnplugged)
                [UIApplication sharedApplication].idleTimerDisabled=YES;
            else [UIApplication sharedApplication].idleTimerDisabled=NO;
		}
	}
	
}
-(IBAction) goPlayer {
	[self.navigationController pushViewController:detailViewControllerIphone animated:(detailViewControllerIphone.mSlowDevice?NO:YES)];
}


/*
 - (id)initWithStyle:(UITableViewStyle)style {
 // Override initWithStyle: if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
 if ((self = [super initWithStyle:style])) {
 }
 return self;
 }
 */

-(bool)startFTPServer {
	int ftpport=0;
	sscanf([ftpPort.text UTF8String],"%d",&ftpport);
	if (ftpport==0) return FALSE;
	
    if (!ftpserver) ftpserver = new CFtpServer();
    bServerRunning = false;
    
    ftpserver->SetMaxPasswordTries( 3 );
	ftpserver->SetNoLoginTimeout( 45 ); // seconds
	ftpserver->SetNoTransferTimeout( 90 ); // seconds
	ftpserver->SetDataPortRange( 1024, 4096 ); // data TCP-Port range = [100-999]
	ftpserver->SetCheckPassDelay( 0 ); // milliseconds. Bruteforcing protection.
	
	pUser = ftpserver->AddUser([ftpUser.text UTF8String], 
							   [ftpPassword.text UTF8String], 
							   [[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/"] UTF8String]);
	
    // Create anonymous user
	if (sc_FTPanonymous.selectedSegmentIndex) {
		pAnonymousUser = ftpserver->AddUser("anonymous", 
                                            NULL, 
                                            [[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/"] UTF8String]);
	}
	
	
    if( pUser ) {
		pUser->SetMaxNumberOfClient( 0 ); // Unlimited
		pUser->SetPrivileges( CFtpServer::READFILE | CFtpServer::WRITEFILE |
							 CFtpServer::LIST | CFtpServer::DELETEFILE | CFtpServer::CREATEDIR |
							 CFtpServer::DELETEDIR );
    }
	if( pAnonymousUser ) pAnonymousUser->SetPrivileges( CFtpServer::READFILE | CFtpServer::WRITEFILE |
													   CFtpServer::LIST | CFtpServer::DELETEFILE | CFtpServer::CREATEDIR |
													   CFtpServer::DELETEDIR );
    if (!ftpserver->StartListening( INADDR_ANY, ftpport )) return false;
    if (!ftpserver->StartAccepting()) return false;
    
    return true;
}

#pragma mark -
#pragma mark View lifecycle

- (void)viewDidLoad {
	clock_t start_time,end_time;	
	start_time=clock();	
    
    //	self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    //	self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
    
	[scrollSettingsView addSubview:cSettingsViewGENPLAYER];
	[scrollSettingsView addSubview:cSettingsViewGENDISPLAY];
	[scrollSettingsView addSubview:cSettingsViewDOWNLOAD];	
	[scrollSettingsView addSubview:cSettingsViewUADE];
	[scrollSettingsView addSubview:cSettingsViewMODPLUG];
    [scrollSettingsView addSubview:cSettingsViewDUMB];
	[scrollSettingsView addSubview:cSettingsViewADPLUG];
	[scrollSettingsView addSubview:cSettingsViewSEXYPSF];
	[scrollSettingsView addSubview:cSettingsViewAOSDK];
    //	[scrollSettingsView addSubview:cSettingsViewGME];
	[scrollSettingsView addSubview:cSettingsViewFTP];
	[scrollSettingsView addSubview:cSettingsViewSID];
    [scrollSettingsView addSubview:cSettingsViewTIM];
	cSettingsViewFTP.hidden=YES;
	cSettingsViewGENPLAYER.hidden=YES;
	cSettingsViewGENDISPLAY.hidden=YES;
	cSettingsViewDOWNLOAD.hidden=YES;	
	cSettingsViewUADE.hidden=YES;
	cSettingsViewMODPLUG.hidden=YES;
    cSettingsViewDUMB.hidden=YES;
	cSettingsViewADPLUG.hidden=YES;
	cSettingsViewSEXYPSF.hidden=YES;
	cSettingsViewAOSDK.hidden=YES;
    //	cSettingsViewGME.hidden=YES;
	cSettingsViewSID.hidden=YES;
    cSettingsViewTIM.hidden=YES;
	
	self.tableView.rowHeight = 32;
	self.tableView.sectionHeaderHeight = 30;
    //	self.tableView.backgroundColor=[UIColor clearColor];
	
	dbop_choice=0;
	
	ftpserver = NULL;
	bServerRunning=0;
	lbl_FTPstatus.text = @"Server is stopped.";
    
    [super viewDidLoad];
    
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
	
	end_time=clock();	
#ifdef LOAD_PROFILE	
	NSLog(@"settings : %d",end_time-start_time);
#endif
}


/*
 - (void)viewWillAppear:(BOOL)animated {
 [super viewWillAppear:animated];
 }
 */
/*
 - (void)viewDidAppear:(BOOL)animated {
 [super viewDidAppear:animated];
 }
 */
/*
 - (void)viewWillDisappear:(BOOL)animated {
 [super viewWillDisappear:animated];
 }
 */
/*
 - (void)viewDidDisappear:(BOOL)animated {
 [super viewDidDisappear:animated];
 }
 */

// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return YES;
}

#pragma mark -
#pragma mark Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    // Return the number of sections.
	return 5;
}


- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    // Return the number of rows in the section.
    if (section==0) return 4;
	if (section==1) return 8;
	if (section==2) return 4;
	if (section==3) return 4;
    if (section==4) return 4;
	return 0;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
	if (section==0) return NSLocalizedString(@"Settings_General",@"");
	if (section==1) return NSLocalizedString(@"Settings_PlaybackLib",@"");
	if (section==2) return NSLocalizedString(@"Settings_DB",@"");
	if (section==3) return NSLocalizedString(@"Settings_LocalFiles",@"");
    if (section==4) return NSLocalizedString(@"Settings_Settings",@"");
	return @"";
}


// Customize the appearance of table view cells.
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    static NSString *CellIdentifier = @"Cell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
    }
    
	if (indexPath.section==0) {
		if (indexPath.row==0) cell.textLabel.text=NSLocalizedString(@"Settings_Player",@"");
		if (indexPath.row==1) cell.textLabel.text=NSLocalizedString(@"Settings_Display",@"");
		if (indexPath.row==2) cell.textLabel.text=NSLocalizedString(@"Settings_Download",@"");
		if (indexPath.row==3) cell.textLabel.text=NSLocalizedString(@"Settings_FTP",@"");
	}
	if (indexPath.section==1) {
		if (indexPath.row==0) cell.textLabel.text=@"MODPLUG";
        if (indexPath.row==1) cell.textLabel.text=@"DUMB";
		if (indexPath.row==2) cell.textLabel.text=@"SID";
        //		if (indexPath.row==3) cell.textLabel.text=@"GME";
		if (indexPath.row==3) cell.textLabel.text=@"UADE";
		if (indexPath.row==4) cell.textLabel.text=@"SexyPSF";
		if (indexPath.row==5) cell.textLabel.text=@"AOSDK";
        if (indexPath.row==6) cell.textLabel.text=@"ADPLUG";
		if (indexPath.row==7) cell.textLabel.text=@"TIMIDITY";
	}
	if (indexPath.section==2) {
		if (indexPath.row==0) cell.textLabel.text=NSLocalizedString(@"Settings_CleanDB",@"");
		if (indexPath.row==1) cell.textLabel.text=NSLocalizedString(@"Settings_ResetPlCnt",@"");
		if (indexPath.row==2) cell.textLabel.text=NSLocalizedString(@"Settings_ResetRatings",@"");
		if (indexPath.row==3) cell.textLabel.text=NSLocalizedString(@"Settings_ResetDB",@"");
	}
	if (indexPath.section==3) {
        if (indexPath.row==0) cell.textLabel.text=NSLocalizedString(@"Settings_RecreateSamples",@"");
		if (indexPath.row==1) cell.textLabel.text=NSLocalizedString(@"Settings_FormatDownloads",@"");
		if (indexPath.row==2) cell.textLabel.text=NSLocalizedString(@"Settings_RemCovers",@"");
        if (indexPath.row==3) cell.textLabel.text=NSLocalizedString(@"Settings_UpdateFilesBackupFlag",@"");
	}
    if (indexPath.section==4) {
        if (indexPath.row==0) cell.textLabel.text=NSLocalizedString(@"Settings_SettingsDefault",@"");
        if (indexPath.row==1) cell.textLabel.text=NSLocalizedString(@"Settings_SettingsHigh",@"");
		if (indexPath.row==2) cell.textLabel.text=NSLocalizedString(@"Settings_SettingsMed",@"");
		if (indexPath.row==3) cell.textLabel.text=NSLocalizedString(@"Settings_SettingsLow",@"");
	}
	
	cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
    
    return cell;
}


/*
 // Override to support conditional editing of the table view.
 - (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
 // Return NO if you do not want the specified item to be editable.
 return YES;
 }
 */


/*
 // Override to support editing the table view.
 - (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
 
 if (editingStyle == UITableViewCellEditingStyleDelete) {
 // Delete the row from the data source
 [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:YES];
 }   
 else if (editingStyle == UITableViewCellEditingStyleInsert) {
 // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
 }   
 }
 */


/*
 // Override to support rearranging the table view.
 - (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
 }
 */


/*
 // Override to support conditional rearranging of the table view.
 - (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
 // Return NO if you do not want the item to be re-orderable.
 return YES;
 }
 */

-(bool) resetRatingsDB {
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
	return TRUE;
}

-(bool) resetPlaycountDB {
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
	return TRUE;
}

-(bool) cleanDB {
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
	
	return TRUE;
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
	if (buttonIndex==1) {
		if (dbop_choice==1) {
			[self cleanDB];
		} else if (dbop_choice==2) {
			[self resetPlaycountDB];
			[[detailViewControllerIphone playlistTabView] reloadData];
		} else if (dbop_choice==3) {
			[self resetRatingsDB];
			[[detailViewControllerIphone playlistTabView] reloadData];
			[detailViewControllerIphone showRating:0];
		} else if (dbop_choice==4) {
			//TODO : should lock DB access during creation..
			[rootViewControllerIphone createEditableCopyOfDatabaseIfNeeded:TRUE quiet:0];
		} else if (dbop_choice==5) { //recreate Samples folder
            [rootViewControllerIphone createSamplesFromPackage:TRUE];
        } else if (dbop_choice==6) {  //format Downloads folder
			NSError *err;
            NSFileManager *mFileMngr=[[NSFileManager alloc] init];
			[mFileMngr 
             removeItemAtPath:[NSString stringWithFormat:@"%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/Downloads/"]]
             error:&err];
			[mFileMngr 
             createDirectoryAtPath:[NSString stringWithFormat:@"%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/Downloads/"]]
             withIntermediateDirectories:TRUE attributes:nil 
             error:&err];	
            [self addSkipBackupAttributeToItemAtPath:[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/Downloads/"]];
            [mFileMngr release];
		} else if (dbop_choice==7) {
            NSError *err;
            NSFileManager *mFileMngr=[[NSFileManager alloc] init];
            if (currentPlayFilepath==nil) return;
            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.jpg",NSHomeDirectory(),[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.png",NSHomeDirectory(),[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.gif",NSHomeDirectory(),[currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.jpg",NSHomeDirectory(),[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.png",NSHomeDirectory(),[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.gif",NSHomeDirectory(),[currentPlayFilepath stringByDeletingPathExtension]] error:&err];
            [mFileMngr release];
            
		} else if (dbop_choice==8) {
            [detailViewControllerIphone performSelectorInBackground:@selector(showWaiting) withObject:nil];
            [self updateFilesDoNotBackupAttributes];  
            [detailViewControllerIphone performSelectorInBackground:@selector(hideWaiting) withObject:nil];
        } else if (dbop_choice==10) { //load default settings
            [detailViewControllerIphone loadDefaultSettings];
        } else if (dbop_choice==11) { //load default settings
            [detailViewControllerIphone loadHighSettings];
        } else if (dbop_choice==12) { //load default settings
            [detailViewControllerIphone loadMedSettings];
        } else if (dbop_choice==13) { //load default settings
            [detailViewControllerIphone loadLowSettings];
        }
	}
}

#pragma mark -
#pragma mark Table view delegate
/*
 - (UIView*) tableView: (UITableView*) tableView
 viewForHeaderInSection: (NSInteger) section
 {
 UIView *customView = [[[UIView alloc] initWithFrame: CGRectMake(0.0, 0.0, tableView.bounds.size.width, 32)] autorelease];
 UILabel *sectionLabel=[[UILabel alloc] initWithFrame:CGRectMake(0,4,tableView.bounds.size.width, 24)];
 sectionLabel.font=[UIFont boldSystemFontOfSize:20];
 if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) { //hack for ipad
 customView.backgroundColor = [UIColor colorWithRed: 0.0 green: 0.0 blue: 0.0 alpha: 1.0];
 
 CAGradientLayer *gradient = [CAGradientLayer layer];
 gradient.frame = CGRectMake(0.0, 0.0, tableView.bounds.size.width, 4);
 gradient.colors = [NSArray arrayWithObjects:(id)[[UIColor colorWithRed: 0.9 green: 0.9 blue: 0.9 alpha: 1.00] CGColor],
 (id)[[UIColor colorWithRed: 0.4 green: 0.4 blue: 0.4 alpha: 1.00] CGColor], nil];
 [customView.layer insertSublayer:gradient atIndex:0];
 
 CAGradientLayer *gradient2 = [CAGradientLayer layer];
 gradient2.frame = CGRectMake(0.0, 28, tableView.bounds.size.width, 4);
 gradient2.colors = [NSArray arrayWithObjects:(id)[[UIColor colorWithRed: 0.4 green: 0.4 blue: 0.4 alpha: 1.00] CGColor],
 (id)[[UIColor colorWithRed: 0.9 green: 0.9 blue: 0.9 alpha: 1.00] CGColor], nil];
 [customView.layer insertSublayer:gradient2 atIndex:0];
 sectionLabel.backgroundColor=[UIColor colorWithRed: 0.0 green: 0.0 blue: 0.0 alpha: 1.0];
 } else {
 customView.backgroundColor = [UIColor clearColor];
 sectionLabel.backgroundColor=[UIColor clearColor];
 }
 
 
 
 switch (section) {
 case 0:
 sectionLabel.text=@"General";
 sectionLabel.textColor = [UIColor colorWithRed:0.8f green:0.8f blue:0.8f alpha:1.0f];
 
 break;
 case 1:
 sectionLabel.text=@"Playback Libraries";
 sectionLabel.textColor = [UIColor colorWithRed:0.8f green:0.8f blue:0.8f alpha:1.0f];
 
 break;
 case 2:
 sectionLabel.text=@"Database";
 sectionLabel.textColor = [UIColor colorWithRed:0.8f green:0.8f blue:0.8f alpha:1.0f];
 
 break;
 case 3:
 sectionLabel.text=@"Local files";
 sectionLabel.textColor = [UIColor colorWithRed:0.8f green:0.8f blue:0.8f alpha:1.0f];
 
 break;
 }
 
 [customView addSubview: sectionLabel];	
 
 
 
 
 return customView;
 }
 */

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    // Navigation logic may go here. Create and push another view controller.
	/*
	 <#DetailViewController#> *detailViewController = [[<#DetailViewController#> alloc] initWithNibName:@"<#Nib name#>" bundle:nil];
     // ...
     // Pass the selected object to the new view controller.
	 [self.navigationController pushViewController:detailViewController animated:YES];
	 [detailViewController release];
	 */
	cSettingsViewGENPLAYER.hidden=YES;
	cSettingsViewGENDISPLAY.hidden=YES;
	cSettingsViewDOWNLOAD.hidden=YES;
	cSettingsViewFTP.hidden=YES;
	cSettingsViewUADE.hidden=YES;
	cSettingsViewMODPLUG.hidden=YES;
    cSettingsViewDUMB.hidden=YES;
	cSettingsViewADPLUG.hidden=YES;
	cSettingsViewSEXYPSF.hidden=YES;
	cSettingsViewAOSDK.hidden=YES;
    //	cSettingsViewGME.hidden=YES;
	cSettingsViewSID.hidden=YES;
    cSettingsViewTIM.hidden=YES;
	if (indexPath.section==0) {
		if (indexPath.row==0)  {
			cSettingsViewGENPLAYER.hidden=NO;
			scrollSettingsView.contentSize = cSettingsViewGENPLAYER.bounds.size;			
		}
		if (indexPath.row==1)  {
			cSettingsViewGENDISPLAY.hidden=NO;
			scrollSettingsView.contentSize = cSettingsViewGENDISPLAY.bounds.size;			
		}
		if (indexPath.row==2)  {
			cSettingsViewDOWNLOAD.hidden=NO;
			scrollSettingsView.contentSize = cSettingsViewDOWNLOAD.bounds.size;			
		}
		if (indexPath.row==3)  {
			cSettingsViewFTP.hidden=NO;
			scrollSettingsView.contentSize = cSettingsViewFTP.bounds.size;
		}
		[scrollSettingsView scrollRectToVisible:CGRectMake(0,0,1,1) animated:NO];
		[self.navigationController pushViewController:settingsView animated:(detailViewControllerIphone.mSlowDevice?NO:YES)];
		[scrollSettingsView flashScrollIndicators];
	}
	if (indexPath.section==1) {
		if (indexPath.row==0) {
			cSettingsViewMODPLUG.hidden=NO;
			scrollSettingsView.contentSize = cSettingsViewMODPLUG.bounds.size;			
		} else
            if (indexPath.row==1) {
                cSettingsViewDUMB.hidden=NO;
                scrollSettingsView.contentSize = cSettingsViewMODPLUG.bounds.size;			
            } else
                if (indexPath.row==2) {
                    cSettingsViewSID.hidden=NO;
                    scrollSettingsView.contentSize = cSettingsViewSID.bounds.size;
                } else
                /*		if (indexPath.row==3) {
                 cSettingsViewGME.hidden=NO;
                 scrollSettingsView.contentSize = cSettingsViewGME.bounds.size;
                 } else	*/
                    if (indexPath.row==3) {
                        cSettingsViewUADE.hidden=NO;
                        scrollSettingsView.contentSize = cSettingsViewUADE.bounds.size;
                    } else
                        if (indexPath.row==4) {
                            cSettingsViewSEXYPSF.hidden=NO;
                            scrollSettingsView.contentSize = cSettingsViewSEXYPSF.bounds.size;
                        } else
                            if (indexPath.row==5) {
                                cSettingsViewAOSDK.hidden=NO;
                                scrollSettingsView.contentSize = cSettingsViewAOSDK.bounds.size;
                            } else
                                if (indexPath.row==6) {
                                    cSettingsViewADPLUG.hidden=NO;
                                    scrollSettingsView.contentSize = cSettingsViewADPLUG.bounds.size;
                                } else
                                    if (indexPath.row==7) {
                                        cSettingsViewTIM.hidden=NO;
                                        scrollSettingsView.contentSize = cSettingsViewTIM.bounds.size;
                                    }
		[scrollSettingsView scrollRectToVisible:CGRectMake(0,0,10,10) animated:NO];
		[self.navigationController pushViewController:settingsView animated:(detailViewControllerIphone.mSlowDevice?NO:YES)];
		[scrollSettingsView flashScrollIndicators];
	}
	if (indexPath.section==2) {
		UIAlertView *msgAlert;
		dbop_choice=0;
		if (indexPath.row==0) {
			dbop_choice=1;
			msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Confirm_DBClean",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
		} else if (indexPath.row==1) {
			dbop_choice=2;
			msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Confirm_Reset_PlCnt",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
		} else if (indexPath.row==2) {
			dbop_choice=3;
			msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Confirm_Reset_Ratings",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
		} else if (indexPath.row==3) {
			dbop_choice=4;
			msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Confirm_ResetDB",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:@"Reset",nil] autorelease];
		}
		[msgAlert show];
	}
	if (indexPath.section==3) {
		UIAlertView *msgAlert;
		dbop_choice=0;
        if (indexPath.row==0) {
			dbop_choice=5;
			msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Confirm_RecreateSamples",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
		}
		else if (indexPath.row==1) {
			dbop_choice=6;
			msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Confirm_FormatDownloads",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
		} else if (indexPath.row==2) {
            currentPlayFilepath =[detailViewControllerIphone getCurrentModuleFilepath];
            if (currentPlayFilepath) {
                dbop_choice=7;
                msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Confirm_RemCurCovers",@""),[currentPlayFilepath lastPathComponent] ] delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
            }
		} else if (indexPath.row==3) {        
            dbop_choice=8;
            msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Confirm_CheckFiles",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];    
        }
		[msgAlert show];
	}
    if (indexPath.section==4) {
		UIAlertView *msgAlert;
		dbop_choice=0;
        if (indexPath.row==0) {
			dbop_choice=10;
			msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Confirm_SettingsDef",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
		}
		else if (indexPath.row==1) {
			dbop_choice=11;
			msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Confirm_SettingsHigh",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
		} else if (indexPath.row==2) {
			dbop_choice=12;
			msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Confirm_SettingsMed",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
		} else if (indexPath.row==3) {
			dbop_choice=13;
			msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Confirm_SettingsLow",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
		}
		[msgAlert show];
	}
    
}


#pragma mark -
#pragma mark Memory management

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Relinquish ownership any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
    // Relinquish ownership of anything that can be recreated in viewDidLoad or on demand.
    // For example: self.myOutlet = nil;
}


- (void)dealloc {
    [super dealloc];
}


@end
