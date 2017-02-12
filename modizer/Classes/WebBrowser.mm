//
//  WebBrowser.m
//  modizer4
//
//  Created by yoyofr on 7/4/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//
extern BOOL is_ios7;

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


@synthesize webView,activityIndicator,backButton,forwardButton,downloadViewController,addressTestField;//,view;
@synthesize detailViewController,toolBar;
@synthesize infoDownloadView,infoDownloadLbl;

-(IBAction) goBookmarks {
    bookmarksVC = [[[WB_BookmarksViewController alloc]  initWithNibName:@"BookmarksViewController" bundle:[NSBundle mainBundle]] autorelease];
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
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:(detailViewController.mSlowDevice?NO:YES)];
    else {
        UIAlertView *nofileplaying=[[[UIAlertView alloc] initWithTitle:@"Warning"
                                                               message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
        [nofileplaying show];
    }
}


-(IBAction) goBack:(id)sender {
	if ([webView canGoBack]) [webView goBack];
	else {
        [self goAbout:sender];
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
        alertChooseName=[[[UIAlertView alloc] initWithTitle:[NSString stringWithFormat:@"Enter Bookmark name for %@",tmpStr] message:nil delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Ok",nil] autorelease];
        [alertChooseName setAlertViewStyle:UIAlertViewStylePlainTextInput];
        UITextField *tf=[alertChooseName textFieldAtIndex:0];
        tf.text=addressTestField.text;
        [alertChooseName show];
	}
}

-(void) deleteBookmark:(int)index {
	if (index>=custom_url_count) return;
	[custom_URL[index] release];
	[custom_URL_name[index] release];
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
	[prefs setObject:valNb forKey:@"Bookmarks_count"];[valNb autorelease];
	for (int i=0;i<custom_url_count;i++) {
		[prefs setObject:custom_URL[i] forKey:[NSString stringWithFormat:@"Bookmark_URL%d",i]];
		[prefs setObject:custom_URL_name[i] forKey:[NSString stringWithFormat:@"Bookmark_URL_name%d",i]];
	}
	
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
        if (lastURL) [lastURL release];
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
        if (lastURL) [lastURL release];
        lastURL=nil;
        if (addressTestField.text==nil) lastURL=nil;
        else lastURL=[[NSString alloc] initWithString:addressTestField.text];
    }
    currentMode=GUIDE_MODE;
    loadStatus=TO_LOAD;
    
//    toolBar.hidden=FALSE;
    CGSize cursize=[self currentSize];
//    webView.frame=CGRectMake(0,44,cursize.width,self.view.frame.size.height-44);
    
	[webView loadHTMLString:EMPTY_PAGE baseURL:nil];
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
    
    [webView loadHTMLString:EMPTY_PAGE baseURL:nil];
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
        if (found_img==2) filename=[NSString stringWithFormat:@"%@/folder.gif",[cover_currentPlayFilepath stringByDeletingLastPathComponent]];
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
    [cover_url_string autorelease];
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
	NSRange r;	
	NSString *MIME = response.MIMEType;
	NSString *appDirectory = [[NSBundle mainBundle] bundlePath];
	NSString *pathMIMETYPESplist = [appDirectory stringByAppendingPathComponent:@"MIMETYPES.plist"];
	NSArray *downloadMIMETypes = [NSArray arrayWithContentsOfFile: pathMIMETYPESplist];
	BOOL asdf = [downloadMIMETypes containsObject:MIME];
    
    //	NSLog(@"Connection : %@",MIME);
	
	if (asdf==NO) {
        
        
        r.location=NSNotFound;
		r=[MIME rangeOfString:@"application/"];
		if (r.location!=NSNotFound) {
            //			NSLog(@"unknown binary content, attempt to download");
            //			NSLog(@"%@",MIME);
			asdf=YES;
		} 
		r.location=NSNotFound;
		r=[MIME rangeOfString:@"binary/"];
		if (r.location!=NSNotFound) {
            //			NSLog(@"unknown binary content, attempt to download");
            //			NSLog(@"%@",MIME);
			asdf=YES;
		}
        r.location=NSNotFound;
		r=[MIME rangeOfString:@"audio/"];
		if (r.location!=NSNotFound) {
            //			NSLog(@"unknown binary content, attempt to download");
            //			NSLog(@"%@",MIME);
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
							NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
							NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
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
                    [fileManager release];
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
							NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
							NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
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
                    [fileManager release];
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
							NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
							NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
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
                    [fileManager release];
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
	frame.origin.y=self.view.frame.size.height-64;
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

- (BOOL)webView:(UIWebView *)webV shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType {
	NSRange r;
	NSString *endUrl=[[[request URL] absoluteString] lastPathComponent];
	
    //NSLog(@"url : %d %@",navigationType,[[request URL] absoluteString]);
	
	if (endUrl==nil) return FALSE;
	
	r.location= NSNotFound;
	r = [[[request URL] absoluteString] rangeOfString:@"modizer://delete_bookmark" options:NSCaseInsensitiveSearch];
	if (r.location != NSNotFound) {
		int i;
		sscanf([[[request URL] absoluteString] UTF8String],"modizer://delete_bookmark%d",&i);
		[self deleteBookmark:i];
		return FALSE;
	}
	if ([[[request URL] absoluteString] compare:@"about:blank"]==NSOrderedSame) return TRUE;
	
	
	//Check for world charts link
	NSURL *url=[request URL];
    
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
						NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
						NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
						[array_label addObject:[localPath lastPathComponent]];
						[array_path addObject:localPath];
						[detailViewController play_listmodules:array_label start_index:0 path:array_path];
					} else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)];
					//[self goPlayer];
				} else { //start download
					[self openPopup: [NSString stringWithFormat:@"Downloading : %@",[localPath lastPathComponent]]];
					
					
					[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:-1 filename:[localPath lastPathComponent] isMODLAND:1 usePrimaryAction:((settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value==2)?1:0)];
				}
                [fileManager release];
				return NO;
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
						NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
						NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
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
                [fileManager release];
				return NO;
			}
		}
	}
	
	
	NSURLConnection *theConnection = [NSURLConnection connectionWithRequest:request delegate:self];
    if (theConnection==nil) {
    	NSLog(@"Connection failed");
    }
	if ((navigationType==UIWebViewNavigationTypeLinkClicked)||
		(navigationType==UIWebViewNavigationTypeReload)||
		(navigationType==UIWebViewNavigationTypeBackForward)) {
		addressTestField.text=[[request URL] absoluteString];
		if ([addressTestField.text caseInsensitiveCompare:@"about:blank"]==NSOrderedSame) addressTestField.text=@"";
        
        NSRange rModizerdb;
        rModizerdb.location=NSNotFound;
        rModizerdb=[addressTestField.text rangeOfString:@"modizerdb.appspot.com" options:NSCaseInsensitiveSearch];
	}
    
    
	return YES;
}

- (void)webViewDidStartLoad:(UIWebView*)webV {
//	[activityIndicator startAnimating];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:YES];
    
    //update addressfield indicator
    UIButton *button = [[UIButton alloc] initWithFrame:CGRectMake(0,0,24,24)];
    [button setImage:[UIImage imageNamed:@"bb_stop.png"] forState:UIControlStateNormal];
    button.imageEdgeInsets = UIEdgeInsetsMake(0, 0, 0, 0);
    [button addTarget:self action:@selector(stopLoading:) forControlEvents:UIControlEventTouchUpInside];
    addressTestField.rightView = button;
    addressTestField.rightViewMode = UITextFieldViewModeUnlessEditing;
    [button release];
    
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

- (void)webViewDidFinishLoad:(UIWebView*)webV {
    
    //update addressfield indicator
    UIButton *button = [[[UIButton alloc] initWithFrame:CGRectMake(0,0,24,24)] autorelease];
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
            if (lastURL) [lastURL release];
            lastURL=nil;
            lastURL=[[NSString alloc] initWithString:addressTestField.text];
			[webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:addressTestField.text]]];
		}
    }
}

- (CGSize)windowSize {
    CGSize size;
    size.width = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.innerWidth"] integerValue];
    size.height = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.innerHeight"] integerValue];
    return size;
}

- (CGPoint)scrollOffset {
    CGPoint pt;
    pt.x = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.pageXOffset"] integerValue];
    pt.y = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.pageYOffset"] integerValue];
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
    CGSize viewSize = [webView frame].size;
    CGSize windowSize = [self windowSize];
    
    CGFloat f = windowSize.width / viewSize.width;
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
    [webView stringByEvaluatingJavaScriptFromString: jsCode];
    
    
    // call js functions
    NSString *tags = [webView stringByEvaluatingJavaScriptFromString:
                      [NSString stringWithFormat:@"getHTMLElementsAtPoint(%li,%li);",(long)(NSInteger)point.x,(long)(NSInteger)point.y]];
    NSString *tagsSRC = [webView stringByEvaluatingJavaScriptFromString:
                         [NSString stringWithFormat:@"getLinkSRCAtPoint(%li,%li);",(long)(NSInteger)point.x,(long)(NSInteger)point.y]];
    
//    NSLog(@"src : %@",tags);
//    NSLog(@"src : %@",tagsSRC);
    
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
                msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Image detected",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Choose_SaveCover",@""),[cover_currentPlayFilepath lastPathComponent]] delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"CoverFolder",@""),NSLocalizedString(@"CoverFile",@""),nil] autorelease];
                
                cover_url_string=[[NSString alloc] initWithString:url];
                cover_expectedContentLength=-1;
                [msgAlert show];
            }
        }
    }
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
	clock_t start_time,end_time;	
	start_time=clock();
    
    lastURL=nil;
    
    bookmarksVC=nil;
    
	self.hidesBottomBarWhenPushed = YES;
    
    UIButton *btn = [[[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)] autorelease];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[[UIBarButtonItem alloc] initWithCustomView: btn] autorelease];
    self.navigationItem.rightBarButtonItem = item;
	
	webView.scalesPageToFit = YES;
	webView.autoresizesSubviews = YES;
	webView.autoresizingMask=(UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth);
    
    UIButton *button = [[UIButton alloc] initWithFrame:CGRectMake(0,0,24,24)];
    [button setImage:[UIImage imageNamed:@"bb_refresh.png"] forState:UIControlStateNormal];
    button.imageEdgeInsets = UIEdgeInsetsMake(0, 0, 0, 0);
    [button addTarget:self action:@selector(refresh:) forControlEvents:UIControlEventTouchUpInside];
    addressTestField.rightView = button;
    addressTestField.rightViewMode = UITextFieldViewModeUnlessEditing;
    [button release];
	
	[[infoDownloadView layer] setCornerRadius:5.0];
	[[infoDownloadView layer] setBorderWidth:2.0];
	[[infoDownloadView layer] setBorderColor:[[UIColor colorWithRed: 0.95f green: 0.95f blue: 0.95f alpha: 1.0f] CGColor]];   //Adding Border color.
	infoDownloadView.hidden=YES;
	
	custom_url_count=0;
	
	[self loadHome];
    [super viewDidLoad];
    
    UITapGestureRecognizer *doubleTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(doubleTap:)];
    doubleTap.numberOfTouchesRequired = 2;
    [self.webView addGestureRecognizer:doubleTap];

	
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"webbro : %d",end_time-start_time);
#endif
}

- (void)viewWillAppear:(BOOL)animated {
    if (!is_ios7) {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleBlack];
    } else {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    }
    
    
    [super viewWillAppear:animated];
    
    [self loadBookmarks];
    bookmarksVC=nil;
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
	[self saveBookmarks];
    if (lastURL) [lastURL release];
    lastURL=nil;
	for (int i=0;i<custom_url_count;i++) {
		[custom_URL[i] release];
		[custom_URL_name[i] release];
	}	
    [super dealloc];
}


@end
