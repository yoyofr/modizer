//
//  SettingsViewController.h
//  modizer4
//
//  Created by yoyofr on 7/23/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"
#import "RootViewControllerIphone.h"

#import "CFtpServer.h"

@class DetailViewControllerIphone;
@class RootViewControllerIphone;


@interface SettingsViewController : UITableViewController <UITextFieldDelegate> {
	//Player
	int mPlayer_ResumeOnLaunch;  // off, on
	int mPlayer_EnqueueMode;    // first, after current, last
	int mPlayer_DefaultAction;  // play, enqueue, enqueue & play
	int mPlayer_ForceMono;  // off, on
	int mPlayer_DefaultMODPlayer; // MODPLUG, UADE, DUMB
	int mPlayer_BackgroundMode;  // off, on/exit on pause, on/fast app switch
	int mPlayer_AutoswitchToPlayerView;  // off, on	
	//Display
	//Download
	//FTP
	//Modplug
	//SID
	//GME
	//UADE
	//SexyPSF
	//AOSDK
	
	IBOutlet DetailViewControllerIphone *detailViewControllerIphone;
	IBOutlet RootViewControllerIphone *rootViewControllerIphone;
	
	IBOutlet UIScrollView *scrollSettingsView;
	IBOutlet UIViewController *settingsView;
	IBOutlet UIView *cSettingsViewMODPLUG,*cSettingsViewDUMB,*cSettingsViewFTP,*cSettingsViewUADE,*cSettingsViewGME;
	IBOutlet UIView *cSettingsViewADPLUG,*cSettingsViewSEXYPSF,*cSettingsViewAOSDK;
	IBOutlet UIView *cSettingsViewGENPLAYER,*cSettingsViewGENDISPLAY,*cSettingsViewDOWNLOAD;
	IBOutlet UIView *cSettingsViewSID,*cSettingsViewTIM;
	
	IBOutlet UITextField *ftpUser,*ftpPassword,*ftpPort;
	
	int dbop_choice;
    IBOutlet UILabel    *lbl_FTPstatus;
	IBOutlet UISegmentedControl *sc_FTPswitch,*sc_FTPanonymous;
	CFtpServer *ftpserver;
    CFtpServer::CUserEntry *pAnonymousUser;
    CFtpServer::CUserEntry *pUser;
    bool bServerRunning;
	
}
@property (nonatomic, retain) IBOutlet UIViewController *settingsView;
@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewControllerIphone;
@property (nonatomic, retain) IBOutlet RootViewControllerIphone *rootViewControllerIphone;
@property (nonatomic, retain) IBOutlet UIScrollView *scrollSettingsView;
@property (nonatomic, retain) IBOutlet UIView *cSettingsViewMODPLUG,*cSettingsViewDUMB,*cSettingsViewFTP,*cSettingsViewUADE,*cSettingsViewGME;
@property (nonatomic, retain) IBOutlet UIView *cSettingsViewADPLUG,*cSettingsViewSEXYPSF,*cSettingsViewAOSDK;
@property (nonatomic, retain) IBOutlet UIView *cSettingsViewGENPLAYER,*cSettingsViewGENDISPLAY,*cSettingsViewDOWNLOAD;
@property (nonatomic, retain) IBOutlet UIView *cSettingsViewSID,*cSettingsViewTIM;
@property (nonatomic, retain) IBOutlet UILabel    *lbl_FTPstatus;
@property (nonatomic, retain) IBOutlet UISegmentedControl *sc_FTPswitch,*sc_FTPanonymous;
@property (nonatomic, retain) IBOutlet UITextField *ftpUser,*ftpPassword,*ftpPort;

-(IBAction) goPlayer;
-(IBAction) FTPswitchChanged;
-(IBAction) FTPanonChanged;

-(bool)startFTPServer;

@end
