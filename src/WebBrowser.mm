//
//  WebBrowser.m
//  modizer4
//
//  Created by yoyofr on 7/4/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#define EMPTY_PAGE @"<html><head><title>Modizer Web Browser</title>\
<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" /></head>\
<body style='background-color:#eee;color:#111;'><div align=\"CENTER\">Loading...</div></body></html>"

#import "WebBrowser.h"
#import "WB_BookmarksViewController.h"
#import "SettingsGenViewController.h"
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

static UIAlertView *alertChooseName;


@synthesize webView,progressIndicator,backButton,forwardButton,downloadViewController,addressTestField;
@synthesize detailViewController,toolBar;
@synthesize infoDownloadView,infoDownloadLbl;

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

-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:YES];
    else {
        UIAlertView *nofileplaying=[[UIAlertView alloc] initWithTitle:@"Warning"
                                                               message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
        [nofileplaying show];
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


-(IBAction) newBookmark:(id)sender {
	if ([addressTestField.text length]) {
        NSString *tmpStr;
        if ([addressTestField.text length]>24) tmpStr=[NSString stringWithFormat:@"%@...",[addressTestField.text substringToIndex:24-3]];
        else tmpStr=[NSString stringWithString:addressTestField.text];
        alertChooseName=[[UIAlertView alloc] initWithTitle:[NSString stringWithFormat:@"Enter Bookmark name for %@",tmpStr] message:nil delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Ok",nil];
        [alertChooseName setAlertViewStyle:UIAlertViewStylePlainTextInput];
        UITextField *tf=[alertChooseName textFieldAtIndex:0];
        tf.text=addressTestField.text;
        [alertChooseName show];
	}
}

-(void) deleteBookmark:(int)index {
	if (index>=custom_url_count) return;
	//[custom_URL[index] release];
	//[custom_URL_name[index] release];
	for (int i=index;i<custom_url_count-1;i++) {
		custom_URL[i]=custom_URL[i+1];
		custom_URL_name[i]=custom_URL_name[i+1];
	}
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
		[prefs setObject:custom_URL[i] forKey:[NSString stringWithFormat:@"Bookmark_URL%d",i]];
		[prefs setObject:custom_URL_name[i] forKey:[NSString stringWithFormat:@"Bookmark_URL_name%d",i]];
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
    UIButton *button = [[UIButton alloc] initWithFrame:CGRectMake(0,0,24,24)];
    [button setImage:[UIImage imageNamed:@"bb_refresh.png"] forState:UIControlStateNormal];
    button.imageEdgeInsets = UIEdgeInsetsMake(0, 0, 0, 0);
    [button addTarget:self action:@selector(refresh:) forControlEvents:UIControlEventTouchUpInside];
    addressTestField.rightView = button;
    addressTestField.rightViewMode = UITextFieldViewModeUnlessEditing;
    
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	
	if ([addressTestField.text caseInsensitiveCompare:@""]==NSOrderedSame) [self loadHome];
	else {
		int doGoogleSearch=0;
		NSRange r0 = [addressTestField.text rangeOfString:@"." options:NSCaseInsensitiveSearch];
		if (r0.location == NSNotFound) {
			NSRange r0 = [addressTestField.text rangeOfString:@"localhost" options:NSCaseInsensitiveSearch];
			if (r0.location == NSNotFound) doGoogleSearch=1;
		}
		if (doGoogleSearch) {
            addressTestField.text=[NSString stringWithFormat:@"http://www.google.com/search?ie=UTF-8&q=%@",[addressTestField.text stringByReplacingOccurrencesOfString:@" " withString:@"+" options:0 range:NSMakeRange(0,[addressTestField.text length])]];
		} else {				
			NSRange r1 = [addressTestField.text rangeOfString:@"http://" options:NSCaseInsensitiveSearch];
			NSRange r2 = [addressTestField.text rangeOfString:@"https://" options:NSCaseInsensitiveSearch];
			NSRange r3 = [addressTestField.text rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
			if ((r1.location == NSNotFound)&&(r2.location == NSNotFound)&&(r3.location == NSNotFound)) {
				addressTestField.text=[NSString stringWithFormat:@"http://%@",addressTestField.text];
			}
		}
        //NSLog(@"yo:%@",addressTestField.text);
		[webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:addressTestField.text]]];
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
        if (addressTestField.text==nil) lastURL=nil;
        else lastURL=[[NSString alloc] initWithString:addressTestField.text];        
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
        if (addressTestField.text==nil) lastURL=nil;
        else lastURL=[[NSString alloc] initWithString:addressTestField.text];
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
        addressTestField.text=[NSString stringWithString:lastURL];
        //[lastURL autorelease];
    } else {[self loadHome];loadStatus=LOADED;}
//	if ([addressTestField.text caseInsensitiveCompare:@""]==NSOrderedSame) [self loadHome];
//	else {
//		[webView loadHTMLString:EMPTY_PAGE baseURL:nil];
//	}
}

-(void)goToURL:(NSString*)address {
    loadStatus=TO_LOAD;
    currentMode=WEB_MODE;
    addressTestField.text=address;
    
    [self textFieldShouldReturn:addressTestField];
}

-(void)goToURLwithLoad:(NSString*)address {
    loadStatus=TO_LOAD;
    currentMode=WEB_MODE;
    addressTestField.text=address;
    
    [self textFieldShouldReturn:addressTestField];
    
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

    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL fileURLWithPath:[[NSBundle mainBundle]
                                                                              pathForResource:@"browser_home" ofType:@"html"]isDirectory:NO]]];
    
//	[webView loadHTMLString:html baseURL:nil];
    addressTestField.text=@"";
}


- (BOOL)alertViewShouldEnableFirstOtherButton:(UIAlertView *)alertView {
    if (alertView==alertChooseName) {
        NSString *inputText = [[alertView textFieldAtIndex:0] text];
        if( [inputText length] >= 1 ) {
            return YES;
        } else {
            return NO;
        }
    } else return YES;
}


- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    int do_save=0;
    NSString *filename;
    NSError *err;
    
    if (alertView==alertChooseName) {
        if (buttonIndex==1) {
        UITextField *name = [alertView textFieldAtIndex:0];
        custom_URL[custom_url_count]=[[NSString alloc] initWithString:addressTestField.text];
		custom_URL_name[custom_url_count]=[[NSString alloc] initWithString:name.text];
		custom_url_count++;
		[self saveBookmarks];
		[self openPopup:@"Bookmark updated"];
        } else return;

    } else {
    if (detailViewController.mPlaylist_size==0) return;
    
	if (buttonIndex==1) { //save as folder cover
        if (found_img==1) filename=[NSString stringWithFormat:@"%@/folder.jpg",[cover_currentPlayFilepath stringByDeletingLastPathComponent]];
        if (found_img==2) filename=[NSString stringWithFormat:@"%@/folder.png",[cover_currentPlayFilepath stringByDeletingLastPathComponent]];
        if (found_img==3) filename=[NSString stringWithFormat:@"%@/folder.gif",[cover_currentPlayFilepath stringByDeletingLastPathComponent]];
        NSFileManager *mFileMngr=[[NSFileManager alloc] init];
        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.jpg",NSHomeDirectory(),[cover_currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.png",NSHomeDirectory(),[cover_currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@/folder.gif",NSHomeDirectory(),[cover_currentPlayFilepath stringByDeletingLastPathComponent]] error:&err];
        do_save=1;
	} else if (buttonIndex==2) { //save as file cover
        
        if (found_img==1) filename=[NSString stringWithFormat:@"%@.jpg",[cover_currentPlayFilepath stringByDeletingPathExtension]];
        if (found_img==2) filename=[NSString stringWithFormat:@"%@.png",[cover_currentPlayFilepath stringByDeletingPathExtension]];
        if (found_img==3) filename=[NSString stringWithFormat:@"%@.gif",[cover_currentPlayFilepath stringByDeletingPathExtension]];
        NSFileManager *mFileMngr=[[NSFileManager alloc] init];
        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.jpg",NSHomeDirectory(),[cover_currentPlayFilepath stringByDeletingPathExtension]] error:&err];
        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.png",NSHomeDirectory(),[cover_currentPlayFilepath stringByDeletingPathExtension]] error:&err];
        [mFileMngr removeItemAtPath:[NSString stringWithFormat:@"%@/%@.gif",NSHomeDirectory(),[cover_currentPlayFilepath stringByDeletingPathExtension]] error:&err];
        do_save=1;
	}
    if (do_save) {
        [self openPopup: [NSString stringWithFormat:@"Downloading : %@",[filename lastPathComponent] ]];
        //NSLog(@"yo %@ / %@ / %d",cover_url_string,filename,cover_expectedContentLength);
        [downloadViewController addURLImageToDownloadList:cover_url_string fileName:filename filesize:cover_expectedContentLength];        
    }
    //[cover_url_string autorelease];
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
	NSRange r;	
	NSString *MIME = response.MIMEType;
	NSString *appDirectory = [[NSBundle mainBundle] bundlePath];
	NSString *pathMIMETYPESplist = [appDirectory stringByAppendingPathComponent:@"MIMETYPES.plist"];
	NSArray *downloadMIMETypes = [NSArray arrayWithContentsOfFile: pathMIMETYPESplist];
	BOOL asdf = [downloadMIMETypes containsObject:MIME];
    
    NSLog(@"Connection : %@",MIME);
        	
	if (asdf==NO) {
        r.location=NSNotFound;
		r=[MIME rangeOfString:@"application/"];
		if (r.location!=NSNotFound) {
            			NSLog(@"unknown binary content, attempt to download");
            			NSLog(@"%@",MIME);
			asdf=YES;
		} 
		r.location=NSNotFound;
		r=[MIME rangeOfString:@"binary/"];
		if (r.location!=NSNotFound) {
            			NSLog(@"unknown binary content, attempt to download");
            			NSLog(@"%@",MIME);
			asdf=YES;
		}
        r.location=NSNotFound;
		r=[MIME rangeOfString:@"audio/"];
		if (r.location!=NSNotFound) {
            			NSLog(@"unknown binary content, attempt to download");
            			NSLog(@"%@",MIME);
			asdf=YES;
		}
        r.location=NSNotFound;
        r=[MIME rangeOfString:@"image/x-mrsid-image"];
        if (r.location!=NSNotFound) {
                        NSLog(@"unknown binary content, attempt to download");
                        NSLog(@"%@",MIME);
            asdf=YES;
        }
	}
	if (asdf == NO) {
	}
	else {
		NSURL *url=[response URL];
		/*NSLog(@"a %@",[url absoluteString]);
         NSLog(@"b %@",response.suggestedFilename);
         NSLog(@"c %d",response.expectedContentLength);
         NSLog(@"d %@",response.textEncodingName);
         */
		//NSLog(@"Download Info: url:%@ name: %@, size:%d, textencoding:%@",[url absoluteString],response.suggestedFilename,response.expectedContentLength,response.textEncodingName);
		suggestedFilename=[NSString stringWithFormat:@"%@",response.suggestedFilename];
		expectedContentLength=response.expectedContentLength;
		[self stopLoading:nil];
		
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
					//NSLog(@"newlocal : %@",localPath);
					//Is it already existing ?
					NSFileManager *fileManager = [[NSFileManager alloc] init];
					BOOL success;
					success = [fileManager fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent: localPath]];
					if (success) {//already existing : start play/enqueue
						if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
							NSMutableArray *array_label = [[NSMutableArray alloc] init ];
							NSMutableArray *array_path = [[NSMutableArray alloc] init];
							[array_label addObject:[localPath lastPathComponent]];
							[array_path addObject:localPath];
							[detailViewController play_listmodules:array_label start_index:0 path:array_path];
						} else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)];
						//[self goPlayer];
					} else { //start download
						[self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
						[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:expectedContentLength
															filename:suggestedFilename isMODLAND:1 usePrimaryAction:((settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0)?1:0)];
					}
                    //[fileManager release];
                    fileManager=nil;
				} else if (isHVSC==1) {  //HVSC DOWNLOAD
					//get modland path to rebuild localPath
					NSString *tmpstr=[ftpPath substringFromIndex:rHVSC.location+10];
					localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",HVSC_BASEDIR,tmpstr];
					//NSLog(@"newlocal : %@",localPath);
					//Is it already existing ?
					NSFileManager *fileManager = [[NSFileManager alloc] init];
					BOOL success;
					success = [fileManager fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent: localPath]];
					if (success) {//already existing : start play/enqueue
						if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
							NSMutableArray *array_label = [[NSMutableArray alloc] init];
							NSMutableArray *array_path = [[NSMutableArray alloc] init];
							[array_label addObject:[localPath lastPathComponent]];
							[array_path addObject:localPath];
							[detailViewController play_listmodules:array_label start_index:0 path:array_path];
						} else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)];
						//[self goPlayer];
					} else { //start download
						[self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
						[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:expectedContentLength
															filename:suggestedFilename isMODLAND:1 usePrimaryAction:((settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0)?1:0)];
					}
                    fileManager=nil;
				}  else if (isASMA==1) {  //ASMA DOWNLOAD
					//get modland path to rebuild localPath
					NSString *tmpstr=[ftpPath substringFromIndex:rASMA.location+6];
					localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",ASMA_BASEDIR,tmpstr];
					//NSLog(@"newlocal : %@",localPath);
					//Is it already existing ?
					NSFileManager *fileManager = [[NSFileManager alloc] init];
					BOOL success;
					success = [fileManager fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent: localPath]];
					if (success) {//already existing : start play/enqueue
						if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
							NSMutableArray *array_label = [[NSMutableArray alloc] init];
							NSMutableArray *array_path = [[NSMutableArray alloc] init];
							[array_label addObject:[localPath lastPathComponent]];
							[array_path addObject:localPath];
							[detailViewController play_listmodules:array_label start_index:0 path:array_path];
						} else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)];
						//[self goPlayer];
					} else { //start download
						[self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
						[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:expectedContentLength
															filename:suggestedFilename isMODLAND:1 usePrimaryAction:((settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0)?1:0)];
					}
                    //[fileManager release];
                    fileManager=nil;
				}else { //STANDARD DOWNLOAD
					localPath=[[NSString alloc] initWithFormat:@"Documents/Downloads/%@",suggestedFilename];
					[self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
					[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:expectedContentLength
														filename:suggestedFilename isMODLAND:0 usePrimaryAction:((settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value==2)?1:0)];
				}
			}
		} else {
			[self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
			[downloadViewController addURLToDownloadList:[url absoluteString] fileName:suggestedFilename filesize:expectedContentLength];
		}
	}
	
	//[receivedDataFromConnection setLength:0];
	[connection cancel];
}

-(void) hidePopup {
	infoDownloadView.hidden=YES;
	mPopupAnimation=0;
}

-(void) openPopup:(NSString *)msg {
	CGRect frame;
    infoDownloadView.layer.zPosition=0xFFFF;
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
    }else{
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
    
    //NSLog(@"url : %d %@",navigationAction.navigationType,[[navigationAction.request URL] absoluteString]);
    
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
                //NSLog(@"newlocal : %@",localPath);
                //Is it already existing ?
                NSFileManager *fileManager = [[NSFileManager alloc] init];
                BOOL success;
                success = [fileManager fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent: localPath]];
                if (success) {//already existing : start play/enqueue
                    if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                        NSMutableArray *array_label = [[NSMutableArray alloc] init];
                        NSMutableArray *array_path = [[NSMutableArray alloc] init];
                        [array_label addObject:[localPath lastPathComponent]];
                        [array_path addObject:localPath];
                        [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                    } else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)];
                    //[self goPlayer];
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
                //NSLog(@"newlocal : %@",localPath);
                //Is it already existing ?
                NSFileManager *fileManager = [[NSFileManager alloc] init];
                BOOL success;
                success = [fileManager fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent: localPath]];
                if (success) {//already existing : start play/enqueue
                    if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                        NSMutableArray *array_label = [[NSMutableArray alloc] init];
                        NSMutableArray *array_path = [[NSMutableArray alloc] init];
                        [array_label addObject:[localPath lastPathComponent]];
                        [array_path addObject:localPath];
                        [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                    } else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)];
                    //[self goPlayer];
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
    
    
    NSURLConnection *theConnection = [NSURLConnection connectionWithRequest:navigationAction.request delegate:self];
    if (theConnection==nil) {
        NSLog(@"Connection failed");
    }
    
    /*WKNavigationTypeLinkActivated,
    WKNavigationTypeFormSubmitted,
    WKNavigationTypeBackForward,
    WKNavigationTypeReload,
    WKNavigationTypeFormResubmitted,
    WKNavigationTypeOther = -1,*/

    if ((navigationAction.navigationType==WKNavigationTypeLinkActivated)||
        (navigationAction.navigationType==WKNavigationTypeReload)||
        (navigationAction.navigationType==WKNavigationTypeBackForward)) {
        addressTestField.text=[[navigationAction.request URL] absoluteString];
        if ([addressTestField.text caseInsensitiveCompare:@"about:blank"]==NSOrderedSame) addressTestField.text=@"";
        
        NSRange rModizerdb;
        rModizerdb.location=NSNotFound;
        rModizerdb=[addressTestField.text rangeOfString:@"modizerdb.appspot.com" options:NSCaseInsensitiveSearch];
        
        [self textFieldShouldReturn:addressTestField];
        NSLog(@"navigating to : %@",addressTestField.text);
        NSLog(@"action: %d",navigationAction.navigationType);
    }
    
    
    decisionHandler(WKNavigationActionPolicyAllow);
    return;
}

//- (void)webViewDidStartLoad:(WKWebView*)webV {
- (void)webView:(WKWebView *)webView
didCommitNavigation:(WKNavigation *)navigation {
//	[activityIndicator startAnimating];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:YES];
    
    //update addressfield indicator
    UIButton *button = [[UIButton alloc] initWithFrame:CGRectMake(0,0,24,24)];
    [button setImage:[UIImage imageNamed:@"bb_stop.png"] forState:UIControlStateNormal];
    button.imageEdgeInsets = UIEdgeInsetsMake(0, 0, 0, 0);
    [button addTarget:self action:@selector(stopLoading:) forControlEvents:UIControlEventTouchUpInside];
    addressTestField.rightView = button;
    addressTestField.rightViewMode = UITextFieldViewModeUnlessEditing;
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
    
    //update addressfield indicator
    UIButton *button = [[UIButton alloc] initWithFrame:CGRectMake(0,0,24,24)];
    [button setImage:[UIImage imageNamed:@"bb_refresh.png"] forState:UIControlStateNormal];
    button.imageEdgeInsets = UIEdgeInsetsMake(0, 0, 0, 0);
    [button addTarget:self action:@selector(refresh:) forControlEvents:UIControlEventTouchUpInside];
    addressTestField.rightView = button;
    addressTestField.rightViewMode = UITextFieldViewModeUnlessEditing;
    
//	[activityIndicator stopAnimating];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:NO];
	if (loadStatus==TO_LOAD) {
        
        if (currentMode==WCHARTS_MODE) {
            loadStatus=LOADED;
            NSString *urlString=[NSString stringWithFormat:@"%@/%@?Device=%s",STATISTICS_URL,WORLDCHARTS_DEFAULTLIST,(detailViewController.mDeviceType==1?"iPad":"iPhone")];
            [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlString]]];
            addressTestField.text=urlString;
        } else if (currentMode==GUIDE_MODE) {
            loadStatus=LOADED;
            NSString *urlString=[NSString stringWithFormat:@"%@/%@?Device=%s",STATISTICS_URL,USERGUIDE_URL,(detailViewController.mDeviceType==1?"iPad":"iPhone")];
            [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlString]]];
            addressTestField.text=urlString;
        } else if (currentMode==WEB_MODE) {
            loadStatus=LOADED;
            
			int doGoogleSearch=0;
			[webView loadHTMLString:EMPTY_PAGE baseURL:nil];
			NSRange r0 = [addressTestField.text rangeOfString:@"." options:NSCaseInsensitiveSearch];
			if (r0.location == NSNotFound) {
				NSRange r0 = [addressTestField.text rangeOfString:@"localhost" options:NSCaseInsensitiveSearch];
				if (r0.location == NSNotFound) doGoogleSearch=1;
			}
			if (doGoogleSearch) {
				addressTestField.text=[NSString stringWithFormat:@"http://www.google.com/search?ie=UTF-8&q=%@",[addressTestField.text stringByReplacingOccurrencesOfString:@" " withString:@"+" options:0 range:NSMakeRange(0,[addressTestField.text length])]];
			} else {				
				NSRange r1 = [addressTestField.text rangeOfString:@"http://" options:NSCaseInsensitiveSearch];
				NSRange r2 = [addressTestField.text rangeOfString:@"https://" options:NSCaseInsensitiveSearch];
				NSRange r3 = [addressTestField.text rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
				if ((r1.location == NSNotFound)&&(r2.location == NSNotFound)&&(r3.location == NSNotFound)) {
					addressTestField.text=[NSString stringWithFormat:@"http://%@",addressTestField.text];
				}
			}
            //if (lastURL) [lastURL release];
            lastURL=nil;
            lastURL=[[NSString alloc] initWithString:addressTestField.text];
			[webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:addressTestField.text]]];
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
                //NSLog(@"result: w %f",pt.x);
                finished1=YES;
            }
        }];
        
    [self.webView evaluateJavaScript:@"window.pageYOffset" completionHandler:^(id _Nullable data, NSError * _Nullable error) {
            if (data) {
                //NSData *data = [NSData dataWithContentsOfURL:[NSURL URLWithString:win_width]];
                pt.y=[(NSNumber*)data integerValue];
                //NSLog(@"result: w %f",pt.y);
                finished2=YES;
            }
        }];
    
    while ((!finished1)||(!finished2))
        {
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
        }
    
    return pt;
}


-(void) doubleTap :(UITapGestureRecognizer*) sender {
    //  <Find HTML tag which was clicked by user>
    //  <If tag is IMG, then get image URL and start saving>
/*    int scrollPositionY = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.pageYOffset"] intValue];
    int scrollPositionX = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.pageXOffset"] intValue];
    int displayWidth = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.innerWidth"] intValue];
    CGFloat scale = webView.frame.size.width / displayWidth;
    CGPoint pt = [sender locationInView:self.webView];
    pt.x /= scale;
    pt.y /= scale;
    pt.x += scrollPositionX;
    pt.y += scrollPositionY;
    NSLog(@"scale:%f displayWidth: %d,x:%f y:%f sx:%f sy:%f",scale,displayWidth, pt.x,pt.y,scrollPositionX,scrollPositionY);
    NSString *js = [NSString stringWithFormat:@"document.elementFromPoint(%f, %f).tagName", pt.x, pt.y];
    NSString *tagName = [self.webView stringByEvaluatingJavaScriptFromString:js];
    NSString *imgURL = [NSString stringWithFormat:@"document.elementFromPoint(%f, %f).src", pt.x, pt.y];
    NSString *urlToSave = [self.webView stringByEvaluatingJavaScriptFromString:imgURL];
    
    NSLog(@"tagName: %@",tagName);
    NSLog(@"urg: %@",urlToSave);*/


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
    
    /*NSString *tags = [webView stringByEvaluatingJavaScriptFromString:
                      [NSString stringWithFormat:@"getHTMLElementsAtPoint(%li,%li);",(long)(NSInteger)point.x,(long)(NSInteger)point.y]];
    NSString *tagsSRC = [webView stringByEvaluatingJavaScriptFromString:
                         [NSString stringWithFormat:@"getLinkSRCAtPoint(%li,%li);",(long)(NSInteger)point.x,(long)(NSInteger)point.y]];
    */

    //NSLog(@"src : %@",tags);
    //NSLog(@"src : %@",tagsSRC);

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
            UIAlertView *msgAlert;  
            cover_currentPlayFilepath =[detailViewController getCurrentModuleFilepath];
            if (cover_currentPlayFilepath) {
                msgAlert=[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Image detected",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Choose_SaveCover",@""),[cover_currentPlayFilepath lastPathComponent]] delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"CoverFolder",@""),NSLocalizedString(@"CoverFile",@""),nil];
                
                cover_url_string=[[NSString alloc] initWithString:url];
                cover_expectedContentLength=-1;
                [msgAlert show];
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
	clock_t start_time,end_time;	
	start_time=clock();
    
    self.navigationController.delegate = self;
    
    forceReloadCells=false;
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
    
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    bottomConstraint=nil;
    
    //self.view.autoresizesSubviews = YES;
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
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:waitingView attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:waitingView attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    views = NSDictionaryOfVariableBindings(addressTestField);
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[addressTestField(32)]" options:0 metrics:nil views:views]];
    //adressfield
    
    addressTestField.translatesAutoresizingMaskIntoConstraints=false;
    progressIndicator.translatesAutoresizingMaskIntoConstraints=false;
    toolBar.translatesAutoresizingMaskIntoConstraints=false;
    
    CGFloat statusbarHeight;
    CGSize statusBarSize = [[UIApplication sharedApplication] statusBarFrame].size;
    statusbarHeight=MIN(statusBarSize.width, statusBarSize.height);
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:addressTestField attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeWidth multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:addressTestField attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    topConstraint=[NSLayoutConstraint constraintWithItem:addressTestField attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeTop multiplier:1.0 constant:statusbarHeight];
    [self.view addConstraint:topConstraint];
    
    //progressbar
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:progressIndicator attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeWidth multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:progressIndicator attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:addressTestField attribute:NSLayoutAttributeBottom multiplier:1.0 constant:0]];
    
    //toolbar
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:toolBar attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeWidth multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:toolBar attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:progressIndicator attribute:NSLayoutAttributeBottom multiplier:1.0 constant:0]];
    
    // Create the new configuration object to set useful options
    WKWebViewConfiguration *configuration = [[WKWebViewConfiguration alloc] init];
    configuration.suppressesIncrementalRendering = NO;//YES;
    if (@available(iOS 10.0, *)) {
        configuration.ignoresViewportScaleLimits = NO;
        configuration.dataDetectorTypes = WKDataDetectorTypeNone;
    }
    

    // Create the new WKWebView
    webView = [[ModizerWebView alloc] initWithFrame:CGRectMake(0,0,0,0) configuration:configuration];
    
    //webView.scalesPageToFit = YES;
    //webView.autoresizesSubviews = YES;
    //webView.autoresizingMask=(UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth);
    webView.translatesAutoresizingMaskIntoConstraints = NO;
    
    // Set the delegate - note this is 'navigationDelegate' not just 'delegate'
    self.webView.navigationDelegate = self;
    
    // Add it to the view
    [self.view addSubview:webView];
    [self.view sendSubviewToBack:webView];
    
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:webView attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeWidth multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:webView attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:toolBar attribute:NSLayoutAttributeBottom multiplier:1.0 constant:0]];
    

    // Load a blank page
    [webView loadHTMLString:@"<html style='margin:0;padding:0;height:100%;width:100%;background:#fff'><body style='margin:0;padding:0;height:100%;width:100%;background:#fff'></body><html>" baseURL:nil];
            
    lastURL=nil;
    
    bookmarksVC=nil;
    
	//self.hidesBottomBarWhenPushed = YES;
    
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithCustomView: btn];
    self.navigationItem.rightBarButtonItem = item;
	
	
    UIButton *button = [[UIButton alloc] initWithFrame:CGRectMake(0,0,24,24)];
    [button setImage:[UIImage imageNamed:@"bb_refresh.png"] forState:UIControlStateNormal];
    button.imageEdgeInsets = UIEdgeInsetsMake(0, 0, 0, 0);
    [button addTarget:self action:@selector(refresh:) forControlEvents:UIControlEventTouchUpInside];
    addressTestField.rightView = button;
    addressTestField.rightViewMode = UITextFieldViewModeUnlessEditing;
    //[button release];
	
	[[infoDownloadView layer] setCornerRadius:5.0];
	[[infoDownloadView layer] setBorderWidth:2.0];
	[[infoDownloadView layer] setBorderColor:[[UIColor colorWithRed: 0.95f green: 0.95f blue: 0.95f alpha: 1.0f] CGColor]];   //Adding Border color.
	infoDownloadView.hidden=YES;
	
	custom_url_count=0;
	
	[self loadHome];
    [super viewDidLoad];
    
    UITapGestureRecognizer *doubleTapMac = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(doubleTap:)];
    //doubleTap.numberOfTouchesRequired = 2;
    doubleTapMac.numberOfTapsRequired=2;
    UITapGestureRecognizer *doubleTapiOS = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(doubleTap:)];
    doubleTapiOS.numberOfTouchesRequired = 2;
    //doubleTap.numberOfTapsRequired=2;
    
    [self.webView addGestureRecognizer:doubleTapiOS];
    [self.webView addGestureRecognizer:doubleTapMac];
    
    [self.webView addObserver:self
                       forKeyPath:NSStringFromSelector(@selector(estimatedProgress))
                          options:0
                          context:nil];


	
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"webbro : %d",end_time-start_time);
#endif
}



- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES;
}

-(void)viewDidDisappear:(BOOL)animated{
    [super viewDidDisappear:animated];
}

- (void)viewWillDisappear:(BOOL)animated {
    [self.navigationController setNavigationBarHidden:NO animated:YES];
    [super viewWillDisappear:animated];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    //if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}

- (void)viewWillLayoutSubviews {
    CGFloat statusbarHeight;
    CGSize statusBarSize = [[UIApplication sharedApplication] statusBarFrame].size;
    statusbarHeight=MIN(statusBarSize.width, statusBarSize.height);
    //NSLog(@"status bar %f %f",statusBarSize.width, statusBarSize.height);
    
    if (topConstraint) [self.view removeConstraint:topConstraint];
    topConstraint=[NSLayoutConstraint constraintWithItem:addressTestField attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:self.view attribute:NSLayoutAttributeTop multiplier:1.0 constant:statusbarHeight];
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
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
    
    self.navigationController.delegate = self;
    
    [self.navigationController setNavigationBarHidden:YES animated:YES];
    
    CGFloat safe_top=0;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"11.0")) {
        if (@available(iOS 11.0, *)) {
            safe_top=[[UIApplication sharedApplication] keyWindow].safeAreaInsets.top;
        }
    }
        
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
    
    [self hideWaiting];
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
    waitingView=nil;
    
	[self saveBookmarks];
    //if (lastURL) [lastURL release];
    lastURL=nil;
	for (int i=0;i<custom_url_count;i++) {
        custom_URL[i]=nil;
        custom_URL_name[i]=nil;
	}	
    //[super dealloc];
}


@end
