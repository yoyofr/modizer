//
//  WebBrowser.m
//  modizer4
//
//  Created by yoyofr on 7/4/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//


static void *WBProgressObserverContext = &WBProgressObserverContext;

#define EMPTY_PAGE @"<html><head><title>Modizer Web Browser</title>\
<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" /></head>\
<body style='background-color:#eee;color:#111;'><div align=\"CENTER\">Loading...</div></body></html>"

#import "WebBrowser.h"
#import "WB_BookmarksViewController.h"
#import "SettingsGenViewController.h"
#import "ModizFileHelper.h"

extern volatile t_settings settings[MAX_SETTINGS];


#define WEB_MODE 0
#define WCHARTS_MODE 1
#define GUIDE_MODE 2
#define WEB_MODE_WITH_LOAD 3

#define TO_LOAD 1
#define LOADED 2

static int currentMode=0;
static int loadStatus=0;
static volatile int mPopupAnimation=0;
static NSString *lastURL=nil;
static WB_BookmarksViewController *bookmarksVC;

@implementation WebBrowser

static NSString *suggestedFilename;
static long long expectedContentLength;
int cover_expectedContentLength;
NSString *cover_url_string,*cover_currentPlayFilepath;
int found_img;

@synthesize webView,progressIndicator,backButton,forwardButton,downloadViewController,addressTextField;
@synthesize detailViewController,toolBar;
@synthesize infoDownloadView,infoDownloadLbl;

@synthesize custom_URL,custom_URL_name;
@synthesize custom_url_count,is_macOS;

#include "MiniPlayerImplementNoTableView.h"

-(void) adjustViewForMiniplayer:(NSNumber*)value {
}

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
        
        if (bottomConstraint) [self.view removeConstraint:bottomConstraint];
        if (wasMiniPlayerOn) bottomConstraint = [NSLayoutConstraint
                                                     constraintWithItem:webView attribute:NSLayoutAttributeBottom
                                                     relatedBy:NSLayoutRelationEqual toItem:miniplayerVC.view attribute:
                                                     NSLayoutAttributeTop multiplier:1.0f constant:0];
        else  bottomConstraint = [NSLayoutConstraint
                                  constraintWithItem:webView attribute:NSLayoutAttributeBottom
                                  relatedBy:NSLayoutRelationEqual toItem:self.view attribute:
                                  NSLayoutAttributeBottom multiplier:1.0f constant:0];
        [self.view addConstraint:bottomConstraint];

        // Force layout update for miniplayer
        [self.view setNeedsLayout];
        [self.view layoutIfNeeded];
    }
}

-(IBAction) goBookmarks {
    bookmarksVC = [[WB_BookmarksViewController alloc]  initWithNibName:@"BookmarksViewController" bundle:[NSBundle mainBundle]];
    //set new title
    bookmarksVC.title = NSLocalizedString(@"Bookmarks",@"");
    bookmarksVC->detailViewController = detailViewController;
    bookmarksVC.modalTransitionStyle = UIModalTransitionStyleCoverVertical;
    bookmarksVC->webBrowser=self;
    // Set new directory
    // And push the window
    [self.navigationController pushViewController:bookmarksVC animated:YES];
//    [self presentViewController:bookmarksVC animated:YES completion:nil];
    
}

#include "AlertsCommonFunctions.h"

-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:YES];
    else {
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"")];
    }
}

-(IBAction) goBackRootVC:(id)sender {
    [self.navigationController popViewControllerAnimated:YES];
}

-(IBAction) goBack:(id)sender {
	if ([webView canGoBack]) [webView goBack];
	else {
        [self goHome:sender];
    }
}

-(IBAction) goForward:(id)sender {
	if ([webView canGoForward]) [webView goForward];
}

-(IBAction) refresh:(id)sender {
    [webView reload];
}

-(void) refresh_webpage {
    if (self.webView.scrollView.refreshControl.refreshing==false) [self.webView.scrollView.refreshControl beginRefreshing];
    [webView reload];
    [self.webView.scrollView.refreshControl endRefreshing];
}


-(IBAction) newBookmark:(id)sender {
    if (custom_url_count<MAX_CUSTOM_URL) {
        if ([addressTextField.text length]) {
            NSString *tmpStr;
            if ([addressTextField.text length]>24) tmpStr=[NSString stringWithFormat:@"%@...",[addressTextField.text substringToIndex:24-3]];
            else tmpStr=[NSString stringWithString:addressTextField.text];
            
            
            UIAlertController *alertC = [UIAlertController alertControllerWithTitle:[NSString stringWithFormat:@"Enter Bookmark name for %@",tmpStr]
                                                                            message:nil
                                                                     preferredStyle:UIAlertControllerStyleAlert];
            __weak UIAlertController *weakAlert = alertC;
            [alertC addTextFieldWithConfigurationHandler:^(UITextField *textField) {
                textField.placeholder = addressTextField.text;
            }];
            
            UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
                
            }];
            [alertC addAction:cancelAction];
            
            UIAlertAction *saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Save",@"") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                UITextField *name = weakAlert.textFields.firstObject;
                [self.custom_URL addObject:[[NSString alloc] initWithString:self.addressTextField.text]];
                [self.custom_URL_name addObject:[[NSString alloc] initWithString:name.text]];
                self.custom_url_count++;
                [self saveBookmarks];
                [self openPopup:@"Bookmark updated"];
            }];
            [alertC addAction:saveAction];
            
            [self showAlert:alertC];
            
        }
    } else {
        UIAlertController *msgAlert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Warning",@"")
                                       message:[NSString stringWithFormat:NSLocalizedString(@"Too many favorites",@""),[cover_currentPlayFilepath lastPathComponent]]
                                       preferredStyle:UIAlertControllerStyleAlert];
        
        
        UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Ok",@"") style:UIAlertActionStyleDefault
           handler:^(UIAlertAction * action) {
            }];
        [msgAlert addAction:cancelAction];
        
        [self presentViewController:msgAlert animated:YES completion:nil];
    }
}

-(void) deleteBookmark:(int)index {
	if (index>=custom_url_count) return;
    [custom_URL removeObjectAtIndex:index];
    [custom_URL_name removeObjectAtIndex:index];
//	for (int i=index;i<custom_url_count-1;i++) {
//		custom_URL[i]=custom_URL[i+1];
//		custom_URL_name[i]=custom_URL_name[i+1];
//	}
	custom_url_count--;
	[self saveBookmarks];
	[self loadHome];
}

-(void) saveBookmarks {
	NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    
	valNb=[[NSNumber alloc] initWithInt:custom_url_count];
	[prefs setObject:valNb forKey:@"Bookmarks_count"];//[valNb autorelease];
	for (int i=0;i<custom_url_count;i++) {
        [prefs setObject:[custom_URL objectAtIndex:i] forKey:[NSString stringWithFormat:@"Bookmark_URL%d",i]];
		[prefs setObject:[custom_URL_name objectAtIndex:i] forKey:[NSString stringWithFormat:@"Bookmark_URL_name%d",i]];
	}
    
    [prefs synchronize];
	
}
-(void) loadBookmarks {
	NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    
    [prefs synchronize];
	
	valNb=[prefs objectForKey:@"Bookmarks_count"];
	if (valNb == nil) custom_url_count = 0;
	else custom_url_count = [valNb intValue];
    if (custom_url_count>MAX_CUSTOM_URL) custom_url_count=MAX_CUSTOM_URL;
    int custom_url_count_tmp=0;
    
    [custom_URL removeAllObjects];
    [custom_URL_name removeAllObjects];
    
    for (int i=0;i<custom_url_count;i++) {
        NSString *tmpstr1,*tmpstr2;
        tmpstr1=[prefs objectForKey:[NSString stringWithFormat:@"Bookmark_URL%d",i]];
        tmpstr2=[prefs objectForKey:[NSString stringWithFormat:@"Bookmark_URL_name%d",i]];
        if (tmpstr1 && tmpstr2) {
            //custom_URL[custom_url_count_tmp]=[[NSString alloc] initWithString:tmpstr1];
            [custom_URL addObject:[[NSString alloc] initWithString:tmpstr1]];
            //custom_URL_name[custom_url_count_tmp]=[[NSString alloc] initWithString:tmpstr2];
            [custom_URL_name addObject:[[NSString alloc] initWithString:tmpstr2]];
            custom_url_count_tmp++;
        }
	}
    custom_url_count=custom_url_count_tmp;
	
}

-(IBAction) goHome:(id)sender {
    loadStatus=0;
    switch (currentMode) {
        case WCHARTS_MODE:[self loadWorldCharts];break;
        case GUIDE_MODE:[self loadUserGuide];break;
        default:
        case WEB_MODE:
            [self loadHome];break;
    }
}

-(IBAction) newUrlEntered:(id)sender {
    
}

-(IBAction) stopLoading:(id)sender {
	[webView stopLoading];
	//[activityIndicator stopAnimating];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:NO];
    
    //update addressfield indicator
    UIButton *button = [[UIButton alloc] initWithFrame:CGRectMake(0,0,28,28)];
    [button setImage:[UIImage imageNamed:@"bb_refresh.png"] forState:UIControlStateNormal];
    button.imageEdgeInsets = UIEdgeInsetsMake(0, -4, 0, 4);
    [button addTarget:self action:@selector(refresh:) forControlEvents:UIControlEventTouchUpInside];
    addressTextField.rightView = button;
    addressTextField.rightViewMode = UITextFieldViewModeUnlessEditing;
    
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	
	if ([addressTextField.text caseInsensitiveCompare:@""]==NSOrderedSame) [self loadHome];
	else {
		int doGoogleSearch=0;
		NSRange r0 = [addressTextField.text rangeOfString:@"." options:NSCaseInsensitiveSearch];
		if (r0.location == NSNotFound) {
			NSRange r0 = [addressTextField.text rangeOfString:@"localhost" options:NSCaseInsensitiveSearch];
			if (r0.location == NSNotFound) doGoogleSearch=1;
		}
		if (doGoogleSearch) {
            addressTextField.text=[NSString stringWithFormat:@"http://www.google.com/search?ie=UTF-8&q=%@",[addressTextField.text stringByReplacingOccurrencesOfString:@" " withString:@"+" options:0 range:NSMakeRange(0,[addressTextField.text length])]];
		} else {				
			NSRange r1 = [addressTextField.text rangeOfString:@"http://" options:NSCaseInsensitiveSearch];
			NSRange r2 = [addressTextField.text rangeOfString:@"https://" options:NSCaseInsensitiveSearch];
			NSRange r3 = [addressTextField.text rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
			if ((r1.location == NSNotFound)&&(r2.location == NSNotFound)&&(r3.location == NSNotFound)) {
				addressTextField.text=[NSString stringWithFormat:@"http://%@",addressTextField.text];
			}
		}
		[webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:addressTextField.text]]];
	}
	return NO;
}

-(CGSize) sizeInOrientation:(UIInterfaceOrientation)orientation
{
    CGSize size = [[UIScreen mainScreen] bounds].size;
    UIApplication *application = [UIApplication sharedApplication];
    if (UIInterfaceOrientationIsLandscape(orientation))
    {
        size = CGSizeMake(size.height, size.width);
    }
    if (application.statusBarHidden == NO)
    {
        size.height -= MIN(application.statusBarFrame.size.width, application.statusBarFrame.size.height);
    }
    return size;
}


-(CGSize) currentSize
{
    return [self sizeInOrientation:[UIApplication sharedApplication].statusBarOrientation];
}



- (void)loadWorldCharts {
    if ((currentMode==WCHARTS_MODE)&&(loadStatus==LOADED)) return;
    if (currentMode==WEB_MODE) { //save WEB url
        //if (lastURL) [lastURL release];
        lastURL=nil;
        if (addressTextField.text==nil) lastURL=nil;
        else lastURL=[[NSString alloc] initWithString:addressTextField.text];        
    }
    currentMode=WCHARTS_MODE;
    loadStatus=TO_LOAD;
    
    //toolBar.hidden=TRUE;
    CGSize cursize=[self currentSize];
//    webView.frame=CGRectMake(0,0,cursize.width,self.view.frame.size.height-44);
	[webView loadHTMLString:EMPTY_PAGE baseURL:nil];
}

- (void)loadUserGuide {
	if ((currentMode==GUIDE_MODE)&&(loadStatus==LOADED)) return;
    if (currentMode==WEB_MODE) { //save WEB url
        //if (lastURL) [lastURL release];
        lastURL=nil;
        if (addressTextField.text==nil) lastURL=nil;
        else lastURL=[[NSString alloc] initWithString:addressTextField.text];
    }
    currentMode=GUIDE_MODE;
    loadStatus=TO_LOAD;
    
//    toolBar.hidden=FALSE;
    CGSize cursize=[self currentSize];
//    webView.frame=CGRectMake(0,44,cursize.width,self.view.frame.size.height-44);
    
	//[webView loadHTMLString:EMPTY_PAGE baseURL:nil];
}

-(void)loadLastURL {
    CGSize cursize=[self currentSize];
//    webView.frame=CGRectMake(0,44,cursize.width,self.view.frame.size.height-44*2);
    
	loadStatus=TO_LOAD;
    currentMode=WEB_MODE;
    
    if (![webView canGoBack]) {
        [self loadHome];
        loadStatus=LOADED;
        return;
    }
	
	if (lastURL) {
        addressTextField.text=[NSString stringWithString:lastURL];
        //[lastURL autorelease];
    } else {[self loadHome];loadStatus=LOADED;}
//	if ([addressTextField.text caseInsensitiveCompare:@""]==NSOrderedSame) [self loadHome];
//	else {
//		[webView loadHTMLString:EMPTY_PAGE baseURL:nil];
//	}
}

-(void)goToURL:(NSString*)address {
    loadStatus=TO_LOAD;
    currentMode=WEB_MODE;
    addressTextField.text=address;
    
    [self textFieldShouldReturn:addressTextField];
}

-(void)goToURLwithLoad:(NSString*)address {
    loadStatus=TO_LOAD;
    currentMode=WEB_MODE;
    addressTextField.text=address;
    
    [self textFieldShouldReturn:addressTextField];
    
    //    toolBar.hidden=FALSE;
    CGSize cursize=[self currentSize];
    //    webView.frame=CGRectMake(0,44,cursize.width,self.view.frame.size.height-44);
    
    //[webView loadHTMLString:EMPTY_PAGE baseURL:nil];
}


- (void)loadHome {
    CGSize cursize=[self currentSize];
//    webView.frame=CGRectMake(0,44,cursize.width,self.view.frame.size.height-44*2);
    //#F627AA
    // v" VERSION_MAJOR_STR "." VERSION_MINOR_STR "
    currentMode=WEB_MODE;

    // Load HTML file and replace localized message
    NSString *htmlPath = [[NSBundle mainBundle] pathForResource:@"browser_home" ofType:@"html"];
    NSString *htmlContent = [NSString stringWithContentsOfFile:htmlPath encoding:NSUTF8StringEncoding error:nil];

    // Replace placeholder with localized message
    NSString *localizedMessage = NSLocalizedString(@"Browser_Welcome_Message", @"");
    htmlContent = [htmlContent stringByReplacingOccurrencesOfString:@"Localized_Message" withString:localizedMessage];

    [webView loadHTMLString:htmlContent baseURL:[NSURL fileURLWithPath:[[NSBundle mainBundle] resourcePath]]];

//	[webView loadHTMLString:html baseURL:nil];
    addressTextField.text=@"";
}


-(void)processURLResponse:(NSURLResponse*)response {
    NSRange r;
    NSString *MIME = response.MIMEType;
    NSString *appDirectory = [[NSBundle mainBundle] bundlePath];
    NSString *pathMIMETYPESplist = [appDirectory stringByAppendingPathComponent:@"MIMETYPES.plist"];
    NSArray *downloadMIMETypes = [NSArray arrayWithContentsOfFile: pathMIMETYPESplist];
    BOOL asdf = [downloadMIMETypes containsObject:MIME];
    
//    MDZILog("process URL response.\nMIME: %@",MIME)
    
    if (asdf==NO) {
        r.location=NSNotFound;
        r=[MIME rangeOfString:@"application/"];
        if (r.location!=NSNotFound) {
                        MDZILog("unknown binary content, attempt to download");
            MDZILog("%@",MIME);
            asdf=YES;
        }
        r.location=NSNotFound;
        r=[MIME rangeOfString:@"binary/"];
        if (r.location!=NSNotFound) {
            MDZILog("unknown binary content, attempt to download");
            MDZILog("%@",MIME);
            asdf=YES;
        }
        r.location=NSNotFound;
        r=[MIME rangeOfString:@"audio/"];
        if (r.location!=NSNotFound) {
            MDZILog("unknown binary content, attempt to download");
            MDZILog("%@",MIME);
            asdf=YES;
        }
        r.location=NSNotFound;
        r=[MIME rangeOfString:@"image/x-mrsid-image"];
        if (r.location!=NSNotFound) {
            MDZILog("unknown binary content, attempt to download");
            MDZILog("%@",MIME);
            asdf=YES;
        }
    }
    if (asdf == NO) {
    }
    else {
        NSURL *url=[response URL];
        suggestedFilename=[NSString stringWithFormat:@"%@",response.suggestedFilename];
        expectedContentLength=response.expectedContentLength;
        
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            [self stopLoading:nil];
        }];
        
        
        //check if FTP or HTTP
        r.location= NSNotFound;
        r = [[url absoluteString] rangeOfString:@"FTP:" options:NSCaseInsensitiveSearch];
        if (r.location != NSNotFound) {  //FTP
            NSString *ftpPath,*ftpHost,*localPath;//,*fileName;
            char tmp_str[1024],*ptr_str;
            //fileName=[endUrl stringByReplacingPercentEscapesUsingEncoding:NSASCIIStringEncoding];
            strcpy(tmp_str,[[[url absoluteString] stringByReplacingPercentEscapesUsingEncoding:NSASCIIStringEncoding] UTF8String]);
            ptr_str=strchr(tmp_str+6,'/');  // 6 first chars are FTP://
            if (ptr_str) {
                *ptr_str=0;
                ptr_str++;
                
                ftpHost=[NSString stringWithFormat:@"%s",tmp_str+6];  //skip the FTP://
                ftpPath=[NSString stringWithFormat:@"/%s",ptr_str];
                
                //Check if it is a collection download (MODLAND, HVSC, ...)
                int isModland=0;
                int isHVSC=0;
                int isASMA=0;
                NSRange rMODLAND;
                rMODLAND.location=NSNotFound;
                rMODLAND=[ftpPath rangeOfString:@"MODLAND" options:NSCaseInsensitiveSearch];
                if (rMODLAND.location!=NSNotFound) isModland++;
                rMODLAND.location=NSNotFound;
                rMODLAND=[ftpPath rangeOfString:@"/pub/modules/" options:NSCaseInsensitiveSearch];
                if (rMODLAND.location!=NSNotFound) isModland++;
                
                NSRange rHVSC;
                rHVSC.location=NSNotFound;
                rHVSC=[ftpPath rangeOfString:@"/C64Music/" options:NSCaseInsensitiveSearch];
                if (rHVSC.location!=NSNotFound) isHVSC++;
                
                NSRange rASMA;
                rASMA.location=NSNotFound;
                rASMA=[ftpPath rangeOfString:@"/ASMA/" options:NSCaseInsensitiveSearch];
                if (rASMA.location!=NSNotFound) isASMA++;
                
                if (isModland==2) {  //MODLAND DOWNLOAD
                    //get modland path to rebuild localPath
                    NSString *tmpstr=[ftpPath substringFromIndex:rMODLAND.location+13];
                    NSString *tmpLocal=DBHelper::getLocalPathFromFullPath(tmpstr);
                    localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,tmpLocal];
                    //Is it already existing ?
                    NSFileManager *fileManager = [[NSFileManager alloc] init];
                    BOOL success;
                    success = [fileManager fileExistsAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent: localPath]];
                    if (success) {//already existing : start play/enqueue
                        if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                            NSMutableArray *array_label = [[NSMutableArray alloc] init ];
                            NSMutableArray *array_path = [[NSMutableArray alloc] init];
                            [array_label addObject:[localPath lastPathComponent]];
                            [array_path addObject:localPath];
                            [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                        } else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)];
                    } else { //start download
                        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                            [self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
                        }];
                        [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:expectedContentLength
                                                            filename:suggestedFilename isMODLAND:1 usePrimaryAction:((settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0)?1:0)];
                    }
                    //[fileManager release];
                    fileManager=nil;
                } else if (isHVSC==1) {  //HVSC DOWNLOAD
                    //get modland path to rebuild localPath
                    NSString *tmpstr=[ftpPath substringFromIndex:rHVSC.location+10];
                    localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",HVSC_BASEDIR,tmpstr];
                    //Is it already existing ?
                    NSFileManager *fileManager = [[NSFileManager alloc] init];
                    BOOL success;
                    success = [fileManager fileExistsAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent: localPath]];
                    if (success) {//already existing : start play/enqueue
                        if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                            NSMutableArray *array_label = [[NSMutableArray alloc] init];
                            NSMutableArray *array_path = [[NSMutableArray alloc] init];
                            [array_label addObject:[localPath lastPathComponent]];
                            [array_path addObject:localPath];
                            [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                        } else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)];
                        
                    } else { //start download
                        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                            [self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
                        }];
                        [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:expectedContentLength
                                                            filename:suggestedFilename isMODLAND:1 usePrimaryAction:((settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0)?1:0)];
                    }
                    fileManager=nil;
                }  else if (isASMA==1) {  //ASMA DOWNLOAD
                    //get modland path to rebuild localPath
                    NSString *tmpstr=[ftpPath substringFromIndex:rASMA.location+6];
                    localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",ASMA_BASEDIR,tmpstr];
                    //Is it already existing ?
                    NSFileManager *fileManager = [[NSFileManager alloc] init];
                    BOOL success;
                    success = [fileManager fileExistsAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent: localPath]];
                    if (success) {//already existing : start play/enqueue
                        if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                            NSMutableArray *array_label = [[NSMutableArray alloc] init];
                            NSMutableArray *array_path = [[NSMutableArray alloc] init];
                            [array_label addObject:[localPath lastPathComponent]];
                            [array_path addObject:localPath];
                            [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                        } else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)];
                        
                    } else { //start download
                        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                            [self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
                        }];
                        [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:expectedContentLength
                                                            filename:suggestedFilename isMODLAND:1 usePrimaryAction:((settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0)?1:0)];
                    }
                    //[fileManager release];
                    fileManager=nil;
                }else { //STANDARD DOWNLOAD
                    localPath=[[NSString alloc] initWithFormat:@"Documents/Downloads/%@",suggestedFilename];
                    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                        [self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
                    }];
                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:expectedContentLength
                                                        filename:suggestedFilename isMODLAND:0 usePrimaryAction:((settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value==2)?1:0)];
                }
            }
        } else {
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                [self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
            }];
            
            [downloadViewController addURLToDownloadList:[url absoluteString] fileName:suggestedFilename filesize:expectedContentLength];
        }
    }
}

-(void) hidePopup {
	infoDownloadView.hidden=YES;
	mPopupAnimation=0;
}

-(void) openPopup:(NSString *)msg {
	CGRect frame;
    infoDownloadView.layer.zPosition=MAXFLOAT;
    infoDownloadLbl.text=[NSString stringWithString:msg];
	if (mPopupAnimation) return;
	mPopupAnimation=1;	
	frame=infoDownloadView.frame;
	frame.origin.y=self.view.frame.size.height;
	infoDownloadView.frame=frame;
	infoDownloadView.hidden=NO;
//	infoDownloadLbl.text=[NSString stringWithString:msg];
	[UIView beginAnimations:nil context:nil];				
	[UIView setAnimationDelay:0];				
	[UIView setAnimationDuration:0.5];
	[UIView setAnimationDelegate:self];
	frame=infoDownloadView.frame;
    frame.origin.y=self.view.frame.size.height-64-32;
	infoDownloadView.frame=frame;
	[UIView setAnimationDidStopSelector:@selector(closePopup)];
	[UIView commitAnimations];
}

-(void) closePopup {
	CGRect frame;
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDelay:1.0];				
	[UIView setAnimationDuration:0.5];
	[UIView setAnimationDelegate:self];	
	frame=infoDownloadView.frame;
	frame.origin.y=self.view.frame.size.height;
	infoDownloadView.frame=frame;
	[UIView setAnimationDidStopSelector:@selector(hidePopup)];
	[UIView commitAnimations];
}


- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation {
    self.progressIndicator.hidden=NO;
}

-(void)observeValueForKeyPath:(NSString *)keyPath
                     ofObject:(id)object
                       change:(NSDictionary<NSKeyValueChangeKey,id> *)change
                      context:(void *)context{
    if (context==WBProgressObserverContext) {
        if ([keyPath isEqualToString:NSStringFromSelector(@selector(estimatedProgress))]
            && object == self.webView) {
            [self.progressIndicator setAlpha:1.0f];
            BOOL animated = self.webView.estimatedProgress > self.progressIndicator.progress;
            [self.progressIndicator setProgress:self.webView.estimatedProgress
                                       animated:animated];
            
            if (self.webView.estimatedProgress >= 1.0f) {
                [UIView animateWithDuration:0.3f
                                      delay:0.3f
                                    options:UIViewAnimationOptionCurveEaseOut
                                 animations:^{
                    [self.progressIndicator setAlpha:0.0f];
                }
                                 completion:^(BOOL finished) {
                    [self.progressIndicator setProgress:0.0f animated:NO];
                }];
            }
        } else {
            [super observeValueForKeyPath:keyPath
                                     ofObject:object
                                       change:change
                                      context:context];
        }
    } else if (context==LoadingProgressObserverContext){
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
        [super observeValueForKeyPath:keyPath
                             ofObject:object
                               change:change
                              context:context];
    }
}



- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
    
  /*NSString *url = [NSString stringWithFormat:@"%@",navigationAction.request.URL.absoluteString];
  BOOL httpRequest = [url containsString:@"http"];
  if (navigationAction.navigationType == WKNavigationTypeLinkActivated && httpRequest) {
    [[UIApplication sharedApplication] openURL:navigationAction.request.URL];
    decisionHandler(WKNavigationActionPolicyCancel);
  } else {
    decisionHandler(WKNavigationActionPolicyAllow);
  }*/
    
    NSRange r;
    NSString *endUrl=[[[navigationAction.request URL] absoluteString] lastPathComponent];
    
    if (endUrl==nil) {
        decisionHandler(WKNavigationActionPolicyCancel);
        return;
    }
    
    r.location= NSNotFound;
    r = [[[navigationAction.request URL] absoluteString] rangeOfString:@"modizer://delete_bookmark" options:NSCaseInsensitiveSearch];
    if (r.location != NSNotFound) {
        int i;
        sscanf([[[navigationAction.request URL] absoluteString] UTF8String],"modizer://delete_bookmark%d",&i);
        [self deleteBookmark:i];
        decisionHandler(WKNavigationActionPolicyCancel);
        return;
    }
    if ([[[navigationAction.request URL] absoluteString] compare:@"about:blank"]==NSOrderedSame) {
        decisionHandler(WKNavigationActionPolicyAllow);
        return;
    }
    
    //Check for world charts link
    NSURL *url=[navigationAction.request URL];
    
    //check if FTP or HTTP
    r.location= NSNotFound;
    r = [[url absoluteString] rangeOfString:@"FTP:" options:NSCaseInsensitiveSearch];
    if (r.location != NSNotFound) {  //FTP
        NSString *ftpPath,*ftpHost,*localPath;
        char tmp_str[1024],*ptr_str;
        strcpy(tmp_str,[[[url absoluteString] stringByReplacingPercentEscapesUsingEncoding:NSASCIIStringEncoding] UTF8String]);
        ptr_str=strchr(tmp_str+6,'/');  // 6 first chars are FTP://
        if (ptr_str) {
            *ptr_str=0;
            ptr_str++;
            
            ftpHost=[NSString stringWithFormat:@"%s",tmp_str+6];  //skip the FTP://
            ftpPath=[NSString stringWithFormat:@"/%s",ptr_str];
            
            //Check if it is a MODLAND or a HVSC download
            int isModland=0;
            int isHVSC=0;
            NSRange rMODLAND;
            rMODLAND.location=NSNotFound;
            rMODLAND=[ftpHost rangeOfString:@"MODLAND" options:NSCaseInsensitiveSearch];
            if (rMODLAND.location!=NSNotFound) isModland++;
            else {
                rMODLAND.location=NSNotFound;
                rMODLAND=[ftpPath rangeOfString:@"MODLAND" options:NSCaseInsensitiveSearch];
                if (rMODLAND.location!=NSNotFound) isModland++;
            }
            rMODLAND.location=NSNotFound;
            rMODLAND=[ftpPath rangeOfString:@"/pub/modules/" options:NSCaseInsensitiveSearch];
            if (rMODLAND.location!=NSNotFound) isModland++;
            
            NSRange rHVSC;
            rHVSC.location=NSNotFound;
            rHVSC=[ftpPath rangeOfString:@"/C64Music/" options:NSCaseInsensitiveSearch];
            if (rHVSC.location!=NSNotFound) isHVSC++;
            
            if (isModland==2) {  //MODLAND DOWNLOAD
                
                
                //get modland path to rebuild localPath
                NSString *tmpstr=[ftpPath substringFromIndex:rMODLAND.location+13];
                NSString *tmpLocal=DBHelper::getLocalPathFromFullPath(tmpstr);
                localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,tmpLocal];
                //Is it already existing ?
                NSFileManager *fileManager = [[NSFileManager alloc] init];
                BOOL success;
                success = [fileManager fileExistsAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent: localPath]];
                if (success) {//already existing : start play/enqueue
                    if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                        NSMutableArray *array_label = [[NSMutableArray alloc] init];
                        NSMutableArray *array_path = [[NSMutableArray alloc] init];
                        [array_label addObject:[localPath lastPathComponent]];
                        [array_path addObject:localPath];
                        [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                    } else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)];
                    
                } else { //start download
                    [self openPopup: [NSString stringWithFormat:@"Downloading : %@",[localPath lastPathComponent]]];
                    
                    
                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:-1 filename:[localPath lastPathComponent] isMODLAND:1 usePrimaryAction:((settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value==2)?1:0)];
                }
                fileManager=nil;
                decisionHandler(WKNavigationActionPolicyCancel);
                return;
            } else if (isHVSC==1) {  //HVSC DOWNLOAD
                //get modland path to rebuild localPath
                NSString *tmpstr=[ftpPath substringFromIndex:rHVSC.location+10];
                localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",HVSC_BASEDIR,tmpstr];
                //Is it already existing ?
                NSFileManager *fileManager = [[NSFileManager alloc] init];
                BOOL success;
                success = [fileManager fileExistsAtPath:[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent: localPath]];
                if (success) {//already existing : start play/enqueue
                    if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                        NSMutableArray *array_label = [[NSMutableArray alloc] init];
                        NSMutableArray *array_path = [[NSMutableArray alloc] init];
                        [array_label addObject:[localPath lastPathComponent]];
                        [array_path addObject:localPath];
                        [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                    } else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)];
                    
                } else { //start download
                    [self openPopup: [NSString stringWithFormat:@"Downloading : %@",[localPath lastPathComponent]]];
                    
                    
                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:-1
                                                        filename:[localPath lastPathComponent] isMODLAND:1 usePrimaryAction:((settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value==2)?1:0)];
                }
                fileManager=nil;
                decisionHandler(WKNavigationActionPolicyCancel);
                return;
            }
        }
    }
    
    //NSURLSession *session = [NSURLSession sharedSession];
    NSURLSessionConfiguration *config = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSURLSession *session = [NSURLSession sessionWithConfiguration:config
                                                          delegate:self
                                                     delegateQueue:nil];
    
    NSURLSessionDataTask *dataTask = [session dataTaskWithRequest:navigationAction.request];
    
    [dataTask resume];
    [session finishTasksAndInvalidate];
    
    if ((navigationAction.navigationType==WKNavigationTypeLinkActivated)||
        (navigationAction.navigationType==WKNavigationTypeReload)||
        (navigationAction.navigationType==WKNavigationTypeBackForward)) {
        
        addressTextField.text=[[navigationAction.request URL] absoluteString];
        if ([addressTextField.text caseInsensitiveCompare:@"about:blank"]==NSOrderedSame) addressTextField.text=@"";
        
        //[self textFieldShouldReturn:addressTextField];
        
        //[webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:addressTextField.text]]];
        
    }
    
    
    decisionHandler(WKNavigationActionPolicyAllow);
    return;
}

- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveResponse:(NSURLResponse *)response completionHandler:(void (^)(NSURLSessionResponseDisposition))completionHandler {
    // Check if it is to be downloaded and in this case add it to the list
    [self processURLResponse:response];
    // Cancel the download by calling the completion handler with Cancel disposition
    completionHandler(NSURLSessionResponseCancel);
    
}


//- (void)webViewDidStartLoad:(WKWebView*)webV {
- (void)webView:(WKWebView *)webView
didCommitNavigation:(WKNavigation *)navigation {
//	[activityIndicator startAnimating];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:YES];
    
    //update addressfield indicator
    UIButton *button = [[UIButton alloc] initWithFrame:CGRectMake(0,0,28,28)];
    [button setImage:[UIImage imageNamed:@"bb_stop.png"] forState:UIControlStateNormal];
    button.imageEdgeInsets = UIEdgeInsetsMake(0, -4, 0, 4);
    [button addTarget:self action:@selector(stopLoading:) forControlEvents:UIControlEventTouchUpInside];
    addressTextField.rightView = button;
    addressTextField.rightViewMode = UITextFieldViewModeUnlessEditing;
    button=nil;
    
    //update back/forward buttons
    UIBarButtonItem *barBtn;
    for (int i=0;i<[toolBar.items count];i++) {
        barBtn=[toolBar.items objectAtIndex:i];
        switch (barBtn.tag) {
            case 1: //back
                if ([webView canGoBack]) {
                    barBtn.enabled=YES;
                } else barBtn.enabled=NO;
                break;
            case 2: //forward
                if ([webView canGoForward]) {
                    barBtn.enabled=YES;
                } else barBtn.enabled=NO;
                break;
        }
    }

}

//- (void)webViewDidFinishLoad:(WKWebView*)webV {
- (void)webView:(WKWebView *)webView
didFinishNavigation:(WKNavigation *)navigation {

    if (is_macOS) {
        // Injecter le JavaScript pour intercepter les clics droits sur les images
        NSString *js = @"\
        document.addEventListener('contextmenu', function(e) { \
            if (e.target.tagName === 'IMG') { \
                e.preventDefault(); \
                window.webkit.messageHandlers.imageContextMenu.postMessage({ \
                    imageUrl: e.target.src, \
                    imageAlt: e.target.alt || '' \
                }); \
                return false; \
            } \
        }, false);";
        
        [webView evaluateJavaScript:js completionHandler:^(id result, NSError *error) {
            if (error) {
                NSLog(@"❌ Erreur injection JS: %@", error);
            }
        }];
    }

    //update addressfield indicator
    UIButton *button = [[UIButton alloc] initWithFrame:CGRectMake(0,0,28,28)];
    [button setImage:[UIImage imageNamed:@"bb_refresh.png"] forState:UIControlStateNormal];
    button.imageEdgeInsets = UIEdgeInsetsMake(0, -4, 0, 4);
    [button addTarget:self action:@selector(refresh:) forControlEvents:UIControlEventTouchUpInside];
    addressTextField.rightView = button;
    addressTextField.rightViewMode = UITextFieldViewModeUnlessEditing;
    
//	[activityIndicator stopAnimating];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:NO];
	if (loadStatus==TO_LOAD) {
        
        if (currentMode==WCHARTS_MODE) {
            loadStatus=LOADED;
            NSString *urlString;
            switch (detailViewController.mDeviceType) {
                case DEVICE_IPHONE://iphone
                    urlString=[NSString stringWithFormat:@"%@/%@?Device=%s",STATISTICS_URL,WORLDCHARTS_DEFAULTLIST,"iPhone"];
                    break;
                case DEVICE_IPAD://ipad
                    urlString=[NSString stringWithFormat:@"%@/%@?Device=%s",STATISTICS_URL,WORLDCHARTS_DEFAULTLIST,"iPad"];
                    break;
                case DEVICE_IPHONE_RETINA://iphone retina
                case DEVICE_IPAD_RETINA://ipad retina
                    urlString=[NSString stringWithFormat:@"%@/%@?Device=%s",STATISTICS_URL,WORLDCHARTS_DEFAULTLIST,"iPad"];
                    break;
                case DEVICE_MACOS://macos
                    urlString=[NSString stringWithFormat:@"%@/%@?Device=%s",STATISTICS_URL,WORLDCHARTS_DEFAULTLIST,"macos"];
                    break;
            }
            
            [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlString]]];
            addressTextField.text=urlString;
        } else if (currentMode==GUIDE_MODE) {
            loadStatus=LOADED;
            NSString *urlString=[NSString stringWithFormat:@"%@/%@?Device=%s",STATISTICS_URL,USERGUIDE_URL,(detailViewController.mDeviceType==1?"iPad":"iPhone")];
            [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlString]]];
            addressTextField.text=urlString;
        } else if (currentMode==WEB_MODE) {
            loadStatus=LOADED;
            
			int doGoogleSearch=0;
			[webView loadHTMLString:EMPTY_PAGE baseURL:nil];
			NSRange r0 = [addressTextField.text rangeOfString:@"." options:NSCaseInsensitiveSearch];
			if (r0.location == NSNotFound) {
				NSRange r0 = [addressTextField.text rangeOfString:@"localhost" options:NSCaseInsensitiveSearch];
				if (r0.location == NSNotFound) doGoogleSearch=1;
			}
			if (doGoogleSearch) {
				addressTextField.text=[NSString stringWithFormat:@"http://www.google.com/search?ie=UTF-8&q=%@",[addressTextField.text stringByReplacingOccurrencesOfString:@" " withString:@"+" options:0 range:NSMakeRange(0,[addressTextField.text length])]];
			} else {				
				NSRange r1 = [addressTextField.text rangeOfString:@"http://" options:NSCaseInsensitiveSearch];
				NSRange r2 = [addressTextField.text rangeOfString:@"https://" options:NSCaseInsensitiveSearch];
				NSRange r3 = [addressTextField.text rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
				if ((r1.location == NSNotFound)&&(r2.location == NSNotFound)&&(r3.location == NSNotFound)) {
					addressTextField.text=[NSString stringWithFormat:@"http://%@",addressTextField.text];
				}
			}
            //if (lastURL) [lastURL release];
            lastURL=nil;
            lastURL=[[NSString alloc] initWithString:addressTextField.text];
			[webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:addressTextField.text]]];
		}
    }
}

- (CGPoint)scrollOffset {
    __block CGPoint pt;
    __block BOOL finished1 = NO;
    __block BOOL finished2 = NO;
    //pt.x = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.pageXOffset"] integerValue];
    //pt.y = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.pageYOffset"] integerValue];
    
    [self.webView evaluateJavaScript:@"window.pageXOffset" completionHandler:^(id _Nullable data, NSError * _Nullable error) {
            if (data) {
                //NSData *data = [NSData dataWithContentsOfURL:[NSURL URLWithString:win_width]];
                pt.x=[(NSNumber*)data integerValue];
                finished1=YES;
            }
        }];
        
    [self.webView evaluateJavaScript:@"window.pageYOffset" completionHandler:^(id _Nullable data, NSError * _Nullable error) {
            if (data) {
                //NSData *data = [NSData dataWithContentsOfURL:[NSURL URLWithString:win_width]];
                pt.y=[(NSNumber*)data integerValue];
                finished2=YES;
            }
        }];
    
    while ((!finished1)||(!finished2))
        {
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
        }
    
    return pt;
}


-(void) findImage :(UITapGestureRecognizer*) sender {
    //  <Find HTML tag which was clicked by user>
    //  <If tag is IMG, then get image URL and start saving>

    if ([sender state]==UIGestureRecognizerStateBegan) {
        CGPoint point = [sender locationInView:self.webView];
        // convert point from view to HTML coordinate system
        CGFloat f = 1/self.webView.scrollView.zoomScale;
        
        if ([[[UIDevice currentDevice] systemVersion] doubleValue] >= 5.) {
            point.x = point.x * f;
            point.y = point.y * f;
        } else {
            // On iOS 4 and previous, document.elementFromPoint is not taking
            // offset into account, we have to handle it
            CGPoint offset = [self scrollOffset];
            point.x = point.x * f + offset.x;
            point.y = point.y * f + offset.y;
        }
        
        // Load the JavaScript code from the Resources and inject it into the web page
        NSString *path = [[NSBundle mainBundle] pathForResource:@"JSTools" ofType:@"js"];
        NSString *jsCode = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
        __block BOOL finished=NO;
        //[webView stringByEvaluatingJavaScriptFromString: jsCode];
        [self.webView evaluateJavaScript:jsCode completionHandler:^(id _Nullable data, NSError * _Nullable error) {
            finished=YES;
        }];
        
        while (!finished)
        {
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
        }
        
        
        
        // call js functions
        finished=NO;
        __block NSString *tags;
        [self.webView evaluateJavaScript:[NSString stringWithFormat:@"getHTMLElementsAtPoint(%li,%li);",(long)(NSInteger)point.x,(long)(NSInteger)point.y] completionHandler:^(id _Nullable data, NSError * _Nullable error) {
            if (data) {
                //NSData *data = [NSData dataWithContentsOfURL:[NSURL URLWithString:win_width]];
                tags=[NSString stringWithString:(NSString*)data];
                finished=YES;
            }
        }];
        
        while (!finished)
        {
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
        }
        
        finished=NO;
        __block NSString *tagsSRC;
        [self.webView evaluateJavaScript:[NSString stringWithFormat:@"getLinkSRCAtPoint(%li,%li);",(long)(NSInteger)point.x,(long)(NSInteger)point.y] completionHandler:^(id _Nullable data, NSError * _Nullable error) {
            if (data) {
                tagsSRC=[NSString stringWithString:(NSString*)data];
                finished=YES;
            }
        }];
        
        while (!finished)
        {
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
        }
        
        NSString *url = nil;
        if ([tags rangeOfString:@",IMG,"].location != NSNotFound) {
            url = tagsSRC;    // Here is the image url!
        }
        
        if (url!=nil) {
            found_img=0;
            
            if ([[url pathExtension] compare:@"jpg" options:NSCaseInsensitiveSearch]==NSOrderedSame) found_img=1; //jpg
            if ([[url pathExtension] compare:@"jpeg" options:NSCaseInsensitiveSearch]==NSOrderedSame) found_img=1; //jpg
            if ([[url pathExtension] compare:@"png" options:NSCaseInsensitiveSearch]==NSOrderedSame) found_img=2; //png
            if ([[url pathExtension] compare:@"gif" options:NSCaseInsensitiveSearch]==NSOrderedSame) found_img=3; //gif
            
            if (found_img) {
                cover_currentPlayFilepath = [detailViewController getCurrentModuleFilepath];
                if (cover_currentPlayFilepath) {
                    UIAlertController *msgAlert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Image detected",@"")
                                                                                      message:[NSString stringWithFormat:NSLocalizedString(@"Choose_SaveCover",@""),[cover_currentPlayFilepath lastPathComponent]]
                                                                               preferredStyle:UIAlertControllerStyleAlert];
                    
                    UIAlertAction* saveCoverFolderAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"CoverFolder",@"") style:UIAlertActionStyleDefault
                                                                                  handler:^(UIAlertAction * action) {
                        if (detailViewController.mPlaylist_size) {
                            NSString *filename;
                            NSError *err;
                            if (found_img==1) filename=[NSString stringWithFormat:@"%@/folder.jpg",[cover_currentPlayFilepath stringByDeletingLastPathComponent]];
                            if (found_img==2) filename=[NSString stringWithFormat:@"%@/folder.png",[cover_currentPlayFilepath stringByDeletingLastPathComponent]];
                            if (found_img==3) filename=[NSString stringWithFormat:@"%@/folder.gif",[cover_currentPlayFilepath stringByDeletingLastPathComponent]];
                            NSFileManager *mFileMngr=[[NSFileManager alloc] init];
                            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.jpg",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
                            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.png",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
                            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.gif",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
                            
                            [self openPopup: [NSString stringWithFormat:@"Downloading : %@",[filename lastPathComponent] ]];
                            [downloadViewController addURLImageToDownloadList:cover_url_string fileName:filename filesize:cover_expectedContentLength];
                        }
                    }];
                    [msgAlert addAction:saveCoverFolderAction];
                    
                    UIAlertAction* saveCoverFileAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"CoverFile",@"") style:UIAlertActionStyleDefault
                                                                                handler:^(UIAlertAction * action) {
                        
                        if (detailViewController.mPlaylist_size) {
                            NSString *filename;
                            NSError *err;
                            if (found_img==1) filename=[NSString stringWithFormat:@"%@.jpg",[cover_currentPlayFilepath stringByDeletingPathExtension]];
                            if (found_img==2) filename=[NSString stringWithFormat:@"%@.png",[cover_currentPlayFilepath stringByDeletingPathExtension]];
                            if (found_img==3) filename=[NSString stringWithFormat:@"%@.gif",[cover_currentPlayFilepath stringByDeletingPathExtension]];
                            NSFileManager *mFileMngr=[[NSFileManager alloc] init];
                            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.jpg",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingPathExtension]] error:&err];
                            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.png",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingPathExtension]] error:&err];
                            [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.gif",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingPathExtension]] error:&err];
                            
                            [self openPopup: [NSString stringWithFormat:@"Downloading : %@",[filename lastPathComponent] ]];
                            [downloadViewController addURLImageToDownloadList:cover_url_string fileName:filename filesize:cover_expectedContentLength];
                        }
                    }];
                    [msgAlert addAction:saveCoverFileAction];
                    
                    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"No",@"") style:UIAlertActionStyleDefault
                                                                         handler:^(UIAlertAction * action) {
                    }];
                    [msgAlert addAction:cancelAction];
                    
                    cover_url_string=[[NSString alloc] initWithString:url];
                    cover_expectedContentLength=-1;
                    
                    [self presentViewController:msgAlert animated:YES completion:nil];
                    //[msgAlert show];
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////


- (WKWebView *)webView:(WKWebView *)webView createWebViewWithConfiguration:(WKWebViewConfiguration *)configuration forNavigationAction:(WKNavigationAction *)navigationAction windowFeatures:(WKWindowFeatures *)windowFeatures
{
  if (!navigationAction.targetFrame.isMainFrame) {
    [webView loadRequest:navigationAction.request];
  }

  return nil;
}

- (void) copyToClip:(NSNotification*)sender {
    if ([[UIPasteboard generalPasteboard] hasImages]) {
        UIImage *myImage=[[UIPasteboard generalPasteboard] image];
        if (myImage) {
            cover_currentPlayFilepath = [detailViewController getCurrentModuleFilepath];
            if (cover_currentPlayFilepath) {
                UIAlertController *msgAlert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Image detected",@"")
                                                                                  message:[NSString stringWithFormat:NSLocalizedString(@"Choose_SaveCover",@""),[cover_currentPlayFilepath lastPathComponent]]
                                                                           preferredStyle:UIAlertControllerStyleAlert];
                
                UIAlertAction* saveCoverFolderAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"CoverFolder",@"") style:UIAlertActionStyleDefault
                                                                              handler:^(UIAlertAction * action) {
                    if (self->detailViewController.mPlaylist_size) {
                        NSString *filename;
                        NSError *err;
                        filename=[NSString stringWithFormat:@"%@/folder.png",[cover_currentPlayFilepath stringByDeletingLastPathComponent]];
                        NSFileManager *mFileMngr=[[NSFileManager alloc] init];
                        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.jpg",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
                        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.png",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
                        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.gif",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
                        
                        [self openPopup: [NSString stringWithFormat:@"Saving : %@",[filename lastPathComponent] ]];
                        NSString *filePath=[NSString stringWithFormat:@"%@/%@/folder.png",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingLastPathComponent]];
                        [mFileMngr createFileAtPath:filePath contents:UIImagePNGRepresentation(myImage)  attributes:NULL];
                        
                        [detailViewController checkNewCover];
                        [self updateMiniPlayer];
                    }
                }];
                [msgAlert addAction:saveCoverFolderAction];
                
                UIAlertAction* saveCoverFileAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"CoverFile",@"") style:UIAlertActionStyleDefault
                                                                            handler:^(UIAlertAction * action) {
                    
                    if (detailViewController.mPlaylist_size) {
                        NSString *filename;
                        NSError *err;
                        filename=[NSString stringWithFormat:@"%@.png",[cover_currentPlayFilepath stringByDeletingPathExtension]];
                        NSFileManager *mFileMngr=[[NSFileManager alloc] init];
                        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.jpg",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingPathExtension]] error:&err];
                        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.png",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingPathExtension]] error:&err];
                        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.gif",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingPathExtension]] error:&err];
                        
                        [self openPopup: [NSString stringWithFormat:@"Saving : %@",[filename lastPathComponent] ]];
                        NSString *filePath=[NSString stringWithFormat:@"%@/%@.png",[ModizFileHelper getAppHomeDirectory],[cover_currentPlayFilepath stringByDeletingPathExtension]];
                        [mFileMngr createFileAtPath:filePath contents:UIImagePNGRepresentation(myImage)  attributes:NULL];
                        
                        [detailViewController checkNewCover];
                        [self updateMiniPlayer];
                    }
                }];
                [msgAlert addAction:saveCoverFileAction];
                
                UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"No",@"") style:UIAlertActionStyleDefault
                                                                     handler:^(UIAlertAction * action) {
                }];
                [msgAlert addAction:cancelAction];
                
                [self presentViewController:msgAlert animated:YES completion:nil];
            }
        }
    }
    
}

//#if TARGET_OS_MACCATALYST
// Implémentation du WKScriptMessageHandler pour gérer les clics sur images
- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)message {

    if ([message.name isEqualToString:@"imageContextMenu"]) {
        NSDictionary *dict = message.body;
        NSString *imageUrl = dict[@"imageUrl"];

        if (imageUrl) {
            // Télécharger et copier l'image
            NSURL *url = [NSURL URLWithString:imageUrl];
            NSURLSessionDataTask *task = [[NSURLSession sharedSession]
                dataTaskWithURL:url
                completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
                    if (data && !error) {
                        UIImage *image = [UIImage imageWithData:data];
                        if (image) {
                            dispatch_async(dispatch_get_main_queue(), ^{
                                [[UIPasteboard generalPasteboard] setImage:image];
                                NSLog(@"✅ Image copiée dans le presse-papier");
                            });
                        }
                    }
                }];
            [task resume];
        }
    }
}
//#endif

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
    self.detailViewController = [self findChildOfClass:[DetailViewControllerIphone class] inTabBarController:tbc];
    self.downloadViewController = [self findChildOfClass:[DownloadViewController class] inTabBarController:tbc];
}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    START_PROFILE
    
    [self loadControllers];
    
    self.navigationController.delegate = self;
    
    custom_URL=[NSMutableArray arrayWithCapacity:MAX_CUSTOM_URL];
    custom_URL_name=[NSMutableArray arrayWithCapacity:MAX_CUSTOM_URL];
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    bottomConstraint=nil;
    
    //self.view.autoresizesSubviews = YES;
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
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:waitingView attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:waitingView attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    views = NSDictionaryOfVariableBindings(addressTextField);
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[addressTextField(32)]" options:0 metrics:nil views:views]];
    //adressfield
    
    waitingViewPlayer = [[WaitingView alloc] init];
    waitingViewPlayer.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingViewPlayer];
    
    views = NSDictionaryOfVariableBindings(waitingViewPlayer);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    addressTextField.translatesAutoresizingMaskIntoConstraints=false;
    progressIndicator.translatesAutoresizingMaskIntoConstraints=false;
    toolBar.translatesAutoresizingMaskIntoConstraints=false;
    
    CGFloat statusbarHeight;
    CGSize statusBarSize = [[UIApplication sharedApplication] statusBarFrame].size;
    statusbarHeight=MIN(statusBarSize.width, statusBarSize.height);
    
//    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:addressTextField attribute:NSLayoutAttributeLeading relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeLeading multiplier:1.0 constant:0]];
//    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:addressTextField attribute:NSLayoutAttributeTrailing relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeTrailing multiplier:1.0 constant:0]];
//    
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:addressTextField attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeWidth multiplier:0.98 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:addressTextField attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    
    topConstraint=[NSLayoutConstraint constraintWithItem:addressTextField attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeTop multiplier:1.0 constant:statusbarHeight];
    [self.view addConstraint:topConstraint];
    
    //progressbar
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:progressIndicator attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeWidth multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:progressIndicator attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:addressTextField attribute:NSLayoutAttributeBottom multiplier:1.0 constant:0]];
    
    //toolbar
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:toolBar attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeWidth multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:toolBar attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:progressIndicator attribute:NSLayoutAttributeBottom multiplier:1.0 constant:0]];
    
    // Create the new configuration object to set useful options
    WKWebViewConfiguration *configuration = [[WKWebViewConfiguration alloc] init];
    configuration.suppressesIncrementalRendering = NO;//YES;
    configuration.ignoresViewportScaleLimits = NO;
    configuration.dataDetectorTypes = WKDataDetectorTypeNone;
    
    is_macOS=false;
    if ([NSProcessInfo processInfo].isiOSAppOnMac) {
        is_macOS=true;
    }
#if TARGET_OS_MACCATALYST
    is_macOS=true;
#endif
    
    if (is_macOS) {
        // ========================================
        // FIX 1: User-Agent pour avoir le bon layout
        // ========================================
        // Forcer le User-Agent de Safari macOS complet
        // configuration.applicationNameForUserAgent = @"Version/17.0 Safari/605.1.15";

        // ========================================
        // FIX 2: Message handler pour copier les images
        // ========================================
        WKUserContentController *contentController = [[WKUserContentController alloc] init];
        [contentController addScriptMessageHandler:self name:@"imageContextMenu"];
        configuration.userContentController = contentController;

        // ========================================
        // FIX 3: Activer les préférences pour les interactions
        // ========================================
        configuration.preferences.javaScriptCanOpenWindowsAutomatically = YES;
        
        // Activer les interactions avec les images
        WKPreferences *preferences = [[WKPreferences alloc] init];
        preferences.javaScriptEnabled = YES;
        configuration.preferences = preferences;
        
        // Autoriser les liens et les previews (important pour le menu contextuel)
        if (@available(macCatalyst 13.0, *)) {
            configuration.defaultWebpagePreferences.allowsContentJavaScript = YES;
        }
    }

    // Create the new WKWebView
    webView = [[ModizerWebView alloc] initWithFrame:CGRectMake(0,0,0,0) configuration:configuration];

    if (is_macOS) {
        // Forcer le User-Agent de Safari macOS pour avoir le bon layout
        webView.customUserAgent = @"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.1 Safari/605.1.15";
    }

    //webView.scalesPageToFit = YES;
    //webView.autoresizesSubviews = YES;
    //webView.autoresizingMask=(UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth);
    webView.translatesAutoresizingMaskIntoConstraints = NO;

    // Set the delegate - note this is 'navigationDelegate' not just 'delegate'
    self.webView.navigationDelegate = self;
    self.webView.UIDelegate=self;
    
    
    // Add it to the view
    [self.view addSubview:webView];
    [self.view sendSubviewToBack:webView];
    
//    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:webView attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeWidth multiplier:1.0 constant:0]];
//    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:webView attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:toolBar attribute:NSLayoutAttributeBottom multiplier:1.0 constant:0]];

    
    // Contraintes Safe Area pour la webView
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:webView attribute:NSLayoutAttributeLeading relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeLeading multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:webView attribute:NSLayoutAttributeTrailing relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeTrailing multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:webView attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:toolBar attribute:NSLayoutAttributeBottom multiplier:1.0 constant:0]];
    
    
    
    // Le bottom sera géré dynamiquement (miniplayer vs safe area) dans viewWillAppear/refreshMiniplayer.
    

    // Load a blank page
    [webView loadHTMLString:@"<html style='margin:0;padding:0;height:100%;width:100%;background:#fff'><body style='margin:0;padding:0;height:100%;width:100%;background:#fff'></body><html>" baseURL:nil];
            
    lastURL=nil;
    
    bookmarksVC=nil;
    
	//self.hidesBottomBarWhenPushed = YES;
    
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:NOW_PLAYING_ICON] style:UIBarButtonItemStylePlain target:self action:@selector(goPlayer)];
    self.navigationItem.rightBarButtonItem = item;
	
	
    UIButton *button = [[UIButton alloc] initWithFrame:CGRectMake(0,0,28,28)];
    [button setImage:[UIImage imageNamed:@"bb_refresh.png"] forState:UIControlStateNormal];
    button.imageEdgeInsets = UIEdgeInsetsMake(0, -4, 0, 4);
    [button addTarget:self action:@selector(refresh:) forControlEvents:UIControlEventTouchUpInside];
    addressTextField.rightView = button;
    addressTextField.rightViewMode = UITextFieldViewModeUnlessEditing;
    //[button release];
	
	[[infoDownloadView layer] setCornerRadius:5.0];
	[[infoDownloadView layer] setBorderWidth:2.0];
	[[infoDownloadView layer] setBorderColor:[[UIColor colorWithRed: 0.95f green: 0.95f blue: 0.95f alpha: 1.0f] CGColor]];   //Adding Border color.
	infoDownloadView.hidden=YES;
	
	custom_url_count=0;
	
	[self loadHome];
    [super viewDidLoad];
    
    if ([NSProcessInfo processInfo].isiOSAppOnMac) {
        UILongPressGestureRecognizer *gestureMac = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(findImage:)];
        //doubleTapMac.numberOfTouchesRequired=2;
        [self.webView addGestureRecognizer:gestureMac];
    }
    
    [self.webView addObserver:self
                       forKeyPath:NSStringFromSelector(@selector(estimatedProgress))
                          options:0
                          context:WBProgressObserverContext];


	
    
    // Initialize the refresh control.
    self.webView.scrollView.refreshControl = [[UIRefreshControl alloc] init];
    self.webView.scrollView.refreshControl.backgroundColor = [UIColor lightGrayColor];
    self.webView.scrollView.refreshControl.tintColor = [UIColor purpleColor];
    [self.webView.scrollView.refreshControl addTarget:self
                                      action:@selector(refresh_webpage)
                            forControlEvents:UIControlEventValueChanged];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(copyToClip:) name:UIPasteboardChangedNotification object:nil];
    
    
END_PROFILE
}


- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES;
}

- (void)viewDidDisappear:(BOOL)animated {
    [repeatingTimer invalidate];
    repeatingTimer = nil;
    
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView removeObserver:self
                                          forKeyPath:observedSelector
                                             context:LoadingProgressObserverContext];
    [super viewDidDisappear:animated];
    
}

- (void)viewWillDisappear:(BOOL)animated {
#if TARGET_OS_MACCATALYST
#else
    [self.navigationController setNavigationBarHidden:NO animated:YES];
#endif
    [super viewWillDisappear:animated];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
#if TARGET_OS_MACCATALYST
#else
    [self.navigationController setNavigationBarHidden:YES animated:YES];
#endif
    
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}

- (void)viewWillLayoutSubviews {
    CGFloat statusbarHeight;
    CGSize statusBarSize = [[UIApplication sharedApplication] statusBarFrame].size;
    statusbarHeight=MIN(statusBarSize.width, statusBarSize.height);
    
    if (topConstraint) [self.view removeConstraint:topConstraint];
    topConstraint=[NSLayoutConstraint constraintWithItem:addressTextField attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeTop multiplier:1.0 constant:statusbarHeight];
    [self.view addConstraint:topConstraint];
    
    [super viewWillLayoutSubviews];
}

- (void)viewWillAppear:(BOOL)animated {
    //[self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
        
    [self loadBookmarks];
    bookmarksVC=nil;
    
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    
    self.navigationController.delegate = self;
    
#if TARGET_OS_MACCATALYST
#else
    [self.navigationController setNavigationBarHidden:YES animated:YES];
#endif
    
    CGFloat safe_top=0;
    safe_top=[[UIApplication sharedApplication] keyWindow].safeAreaInsets.top;
    
    if ([detailViewController mPlaylist_size]>0) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    } else {
        wasMiniPlayerOn=false;
        [self hideMiniPlayer];
    }
    
    if (bottomConstraint) [self.view removeConstraint:bottomConstraint];
    if (wasMiniPlayerOn) bottomConstraint = [NSLayoutConstraint
                                                 constraintWithItem:webView attribute:NSLayoutAttributeBottom
                                                 relatedBy:NSLayoutRelationEqual toItem:miniplayerVC.view attribute:
                                                 NSLayoutAttributeTop multiplier:1.0f constant:0];
    else  bottomConstraint = [NSLayoutConstraint
                              constraintWithItem:webView attribute:NSLayoutAttributeBottom
                              relatedBy:NSLayoutRelationEqual toItem:self.view attribute:
                              NSLayoutAttributeBottom multiplier:1.0f constant:0];
    
    [self.view addConstraint:bottomConstraint];
    
    // Force layout update for miniplayer
    [self.view setNeedsLayout];
    [self.view layoutIfNeeded];

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
    
    [super viewWillAppear:animated];
}
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    if (bookmarksVC) [bookmarksVC shouldAutorotateToInterfaceOrientation:interfaceOrientation];
	return YES;
}

- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
    //    return UIInterfaceOrientationMaskAllButUpsideDown;
}

- (BOOL)shouldAutorotate{
    return TRUE;
}

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void)dealloc {
    [waitingView removeFromSuperview];
    [waitingViewPlayer removeFromSuperview];
    waitingView=nil;
    waitingViewPlayer=nil;
    
	[self saveBookmarks];
    //if (lastURL) [lastURL release];
    lastURL=nil;
	for (int i=0;i<custom_url_count;i++) {
        //custom_URL[i]=nil;
        //custom_URL_name[i]=nil;
	}
    [custom_URL removeAllObjects];
    [custom_URL_name removeAllObjects];
    //[super dealloc];
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


@end
