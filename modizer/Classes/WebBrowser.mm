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

#define WEB_MODE 0
#define WCHARTS_MODE 1
#define GUIDE_MODE 2

#define TO_LOAD 1
#define LOADED 2

static int currentMode=0;
static int loadStatus=0;
static volatile int mPopupAnimation=0;
static NSString *lastURL=nil;

@implementation WebBrowser

@synthesize webView,activityIndicator,backButton,forwardButton,downloadViewController,addressTestField;//,view;
@synthesize detailViewController,toolBar;
@synthesize infoDownloadView,infoDownloadLbl;

-(IBAction) goPlayer {
	[self.navigationController pushViewController:detailViewController animated:(detailViewController.mSlowDevice?NO:YES)];
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


-(IBAction) newBookmark:(id)sender {
	if ([addressTestField.text length]) {
		custom_URL[custom_url_count]=[[NSString alloc] initWithString:addressTestField.text];
		custom_URL_name[custom_url_count]=[[NSString alloc] initWithString:addressTestField.text];
		custom_url_count++;
		[self saveBookmarks];
		
		
		[self openPopup:@"Bookmark updated"];
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
	custom_url_count = [valNb intValue];
	for (int i=0;i<custom_url_count;i++) {
		custom_URL[i]=[[NSString alloc] initWithString:[prefs objectForKey:[NSString stringWithFormat:@"Bookmark_URL%d",i]]];
		custom_URL_name[i]=[[NSString alloc] initWithString:[prefs objectForKey:[NSString stringWithFormat:@"Bookmark_URL_name%d",i]]];
	}
	
}

-(IBAction) goAbout:(id)sender {
    loadStatus=0;
    switch (currentMode) {
        case WCHARTS_MODE:[self loadWorldCharts];break;
        case GUIDE_MODE:[self loadUserGuide];break;
        default:
        case WEB_MODE:[self loadHome];break;
    }
}

-(IBAction) newUrlEntered:(id)sender {
    
}

-(IBAction) stopLoading:(id)sender {
	[webView stopLoading];
	[activityIndicator stopAnimating];
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
        if (addressTestField.text==nil) lastURL=nil;
        else lastURL=[[NSString alloc] initWithString:addressTestField.text];        
    }
    currentMode=WCHARTS_MODE;
    loadStatus=TO_LOAD;
    
/*	UIBarButtonItem *barBtn=[toolBar.items objectAtIndex:0];
	//barBtn.enabled=NO;
	barBtn=[toolBar.items objectAtIndex:1];
	barBtn.enabled=NO;
	//barBtn=[toolBar.items objectAtIndex:2];
	//barBtn.enabled=NO;
 */
    toolBar.hidden=TRUE;
    CGSize cursize=[self currentSize];
    webView.frame=CGRectMake(0,0,cursize.width,self.view.frame.size.height-44);
	[webView loadHTMLString:EMPTY_PAGE baseURL:nil];
}

- (void)loadUserGuide {
	if ((currentMode==GUIDE_MODE)&&(loadStatus==LOADED)) return;
    if (currentMode==WEB_MODE) { //save WEB url
        if (addressTestField.text==nil) lastURL=nil;
        else lastURL=[[NSString alloc] initWithString:addressTestField.text];
    }
    currentMode=GUIDE_MODE;
    loadStatus=TO_LOAD;
    
    toolBar.hidden=FALSE;
    CGSize cursize=[self currentSize];
    webView.frame=CGRectMake(0,44,cursize.width,self.view.frame.size.height-44);
    
    UIBarButtonItem *barBtn=[toolBar.items objectAtIndex:0];
	//barBtn.enabled=NO;
	barBtn=[toolBar.items objectAtIndex:1];
	barBtn.enabled=NO;
	//barBtn=[toolBar.items objectAtIndex:2];
	//barBtn.enabled=NO;
	[webView loadHTMLString:EMPTY_PAGE baseURL:nil];
}

-(void)loadLastURL {
    toolBar.hidden=FALSE;
    CGSize cursize=[self currentSize];
    webView.frame=CGRectMake(0,44,cursize.width,self.view.frame.size.height-44*2);
    
	UIBarButtonItem *barBtn=[toolBar.items objectAtIndex:0];
	//barBtn.enabled=YES;
	barBtn=[toolBar.items objectAtIndex:1];
	barBtn.enabled=YES;
	//barBtn=[toolBar.items objectAtIndex:2];
	//barBtn.enabled=YES;
	loadStatus=TO_LOAD;
    currentMode=WEB_MODE;
    
    if (![webView canGoBack]) {
        [self loadHome];
        return;
    }
	
	if (lastURL) {
        addressTestField.text=[NSString stringWithString:lastURL];
        [lastURL autorelease];
    } else [self loadHome];
//	if ([addressTestField.text caseInsensitiveCompare:@""]==NSOrderedSame) [self loadHome];
//	else {
//		[webView loadHTMLString:EMPTY_PAGE baseURL:nil];
//	}
}

- (void)loadHome {
    toolBar.hidden=FALSE;
    CGSize cursize=[self currentSize];
    webView.frame=CGRectMake(0,44,cursize.width,self.view.frame.size.height-44*2);
    currentMode=WEB_MODE;
	NSString *html = @"<html><head><title>Modizer Web Browser</title></head>\
	<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" />\
	<body><div><b><font size=+1>Modizer v" VERSION_MAJOR_STR "." VERSION_MINOR_STR "</font></b><br><font size=-1><i>by Yohann Magnien / YoyoFR</i></font>\
	&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\
	<a href=""http://yoyofr.blogspot.com/p/modizer.html"" style=""color:#404020;"">website</a><hr>";
	
	if (custom_url_count) {
		html=[NSString stringWithFormat:@"%@%@",html,@"<p><b>Bookmarks</b><br>"];	
		for (int i=0;i<custom_url_count;i++) {
			html=[NSString stringWithFormat:@"%@<a href='modizer://delete_bookmark%d'><font color=#E00000>del</font></a>&nbsp<a href='%@'>%@</a><br>",html,i,custom_URL[i],custom_URL_name[i]];
		}
	} else {
		html=[NSString stringWithFormat:@"%@<p><b>No bookmark.</b><br></p>",html];
	}
	html=[NSString stringWithFormat:@"%@%@",html,@"<p><b>Modules</b> style music (Amiga, PC, demos...)<br>\
          <a href='http://www.exotica.org.uk/'>http://www.exotica.org.uk</a><br>\
          <a href='http://amp.dascene.net/'>http://amp.dascene.net</a><br>\
          <a href='http://modarchive.org/'>http://modarchive.org</a><br>\
          <a href='http://www.amigascne.org'>http://www.amigascne.org</a><br>\
          <a href='http://chiptunes.org/'>http://chiptunes.org</a><br>\
          <a href='http://chiptunes.back2roots.org/'>http://chiptunes.back2roots.org</a><br>\
          <a href='http://2a03.free.fr/'>http://2a03.free.fr</a><br>\
          <a href='http://chipcovers.free.fr/'>http://chipcovers.free.fr</a><br>\
          <a href='http://www.mirsoft.info/gamemods-archive.php'>http://www.mirsoft.info/gamemods-archive.php</a><br>\
          <a href='http://www.scene.org/dir.php?dir=/music/'>http://www.scene.org/dir.php?dir=/music</a><br></p>\
          <p><b>Midi</b> music (classical, pop, games,...)<br>\
          <a href='http://www.lvbeethoven.com/Midi/index.html'>http://www.lvbeethoven.com/Midi/index.html</a><br>\
          <a href='http://www.midishrine.com/'>http://www.midishrine.com</a><br>\
          <a href='http://www.vgmusic.com/'>http://www.vgmusic.com</a><br>\
          <a href='http://www.aganazzar.com/midi.html'>http://www.aganazzar.com/midi.html</a><br>\
          <a href='http://www.mirsoft.info/gamemids-archive.php'>http://www.mirsoft.info/gamemids-archive.php</a><br></p>\
          </div></body></html>"];  
	[webView loadHTMLString:html baseURL:nil];  
    //	addressTestField.text=@"";
}

static NSString *suggestedFilename;
static long long expectedContentLength;
int cover_expectedContentLength;
NSString *cover_url_string,*cover_currentPlayFilepath;
int found_img;

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    int do_save=0;
    NSString *filename;
    NSError *err;
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
				
				//Check if it is a MODLAND or a HVSC download
				int isModland=0;
				int isHVSC=0;
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
						if (detailViewController.sc_DefaultAction.selectedSegmentIndex==0) {						
							NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
							NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
							[array_label addObject:[localPath lastPathComponent]];
							[array_path addObject:localPath];
							[detailViewController play_listmodules:array_label start_index:0 path:array_path];
						} else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(detailViewController.sc_DefaultAction.selectedSegmentIndex==1)];
						//[self goPlayer];
					} else { //start download
						[self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
						[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:expectedContentLength
															filename:suggestedFilename isMODLAND:1 usePrimaryAction:((detailViewController.sc_DefaultAction.selectedSegmentIndex==0)?1:0)];
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
						if (detailViewController.sc_DefaultAction.selectedSegmentIndex==0) {						
							NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
							NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
							[array_label addObject:[localPath lastPathComponent]];
							[array_path addObject:localPath];
							[detailViewController play_listmodules:array_label start_index:0 path:array_path];
						} else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(detailViewController.sc_DefaultAction.selectedSegmentIndex==1)];
						//[self goPlayer];
					} else { //start download
						[self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
						[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:expectedContentLength
															filename:suggestedFilename isMODLAND:1 usePrimaryAction:((detailViewController.sc_DefaultAction.selectedSegmentIndex==0)?1:0)];
					}
                    [fileManager release];
				} else { //STANDARD DOWNLOAD
					localPath=[[NSString alloc] initWithFormat:@"Documents/Downloads/%@",suggestedFilename];
					[self openPopup: [NSString stringWithFormat:@"Downloading : %@",suggestedFilename]];
					[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:expectedContentLength
														filename:suggestedFilename isMODLAND:0 usePrimaryAction:((detailViewController.sc_DefaultAction.selectedSegmentIndex==0)?1:0)];
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
	if (mPopupAnimation) return;
	mPopupAnimation=1;	
	frame=infoDownloadView.frame;
	frame.origin.y=self.view.frame.size.height;
	infoDownloadView.frame=frame;
	infoDownloadView.hidden=NO;
	infoDownloadLbl.text=[NSString stringWithString:msg];
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
	
    //	NSLog(@"url : %d %@",navigationType,[[request URL] absoluteString]);
	
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
			rMODLAND=[ftpPath rangeOfString:@"MODLAND" options:NSCaseInsensitiveSearch];
			if (rMODLAND.location!=NSNotFound) isModland++;
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
					if (detailViewController.sc_DefaultAction.selectedSegmentIndex==0) {						
						NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
						NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
						[array_label addObject:[localPath lastPathComponent]];
						[array_path addObject:localPath];
						[detailViewController play_listmodules:array_label start_index:0 path:array_path];
					} else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(detailViewController.sc_DefaultAction.selectedSegmentIndex==1)]; 
					//[self goPlayer];
				} else { //start download
					[self openPopup: [NSString stringWithFormat:@"Downloading : %@",[localPath lastPathComponent]]];
					
					
					[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:-1
														filename:[localPath lastPathComponent] isMODLAND:1 usePrimaryAction:((detailViewController.sc_DefaultAction.selectedSegmentIndex==0)?1:0)];
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
					if (detailViewController.sc_DefaultAction.selectedSegmentIndex==0) {
						NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
						NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
						[array_label addObject:[localPath lastPathComponent]];
						[array_path addObject:localPath];
						[detailViewController play_listmodules:array_label start_index:0 path:array_path];
					} else [detailViewController add_to_playlist:localPath fileName:[localPath lastPathComponent] forcenoplay:(detailViewController.sc_DefaultAction.selectedSegmentIndex==1)];
					//[self goPlayer];
				} else { //start download
					[self openPopup: [NSString stringWithFormat:@"Downloading : %@",[localPath lastPathComponent]]];
					
					
					[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:ftpHost filesize:-1
														filename:[localPath lastPathComponent] isMODLAND:1 usePrimaryAction:((detailViewController.sc_DefaultAction.selectedSegmentIndex==0)?1:0)];
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
        if (rModizerdb.location!=NSNotFound) {
            if (toolBar.hidden==FALSE) {
                toolBar.hidden=TRUE;
                CGSize cursize=[self currentSize];
                webView.frame=CGRectMake(0,0,cursize.width,self.view.frame.size.height);
            }
        } else {
            if (toolBar.hidden) {
                toolBar.hidden=FALSE;
                CGSize cursize=[self currentSize];
                webView.frame=CGRectMake(0,44,cursize.width,self.view.frame.size.height-44*2);
            }
        }
        
        
	}
	return YES;
}

- (void)webViewDidStartLoad:(UIWebView*)webV {
	[activityIndicator startAnimating];
}

- (void)webViewDidFinishLoad:(UIWebView*)webV {
	[activityIndicator stopAnimating];
	if (loadStatus==TO_LOAD) {
        
        if (currentMode==WCHARTS_MODE) {
            loadStatus=LOADED;
            NSString *urlString=[NSString stringWithFormat:@"%@/%@?Device=%s",STATISTICS_URL,WORLDCHARTS_DEFAULTLIST,(detailViewController.mDeviceType==1?"iPad":"iPhone")];
            [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlString]]];
            //addressTestField.text=urlString;
        } else if (currentMode==GUIDE_MODE) {
            loadStatus=LOADED;
            NSString *urlString=[NSString stringWithFormat:@"%@/%@?Device=%s",STATISTICS_URL,USERGUIDE_URL,(detailViewController.mDeviceType==1?"iPad":"iPhone")];
            [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlString]]];
            //addressTestField.text=urlString;        
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
			[webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:addressTestField.text]]];
		}
        
    }
}

-(void) doubleTap :(UITapGestureRecognizer*) sender {
    //  <Find HTML tag which was clicked by user>
    //  <If tag is IMG, then get image URL and start saving>
    int scrollPositionY = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.pageYOffset"] intValue];
    int scrollPositionX = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.pageXOffset"] intValue];
    int displayWidth = [[self.webView stringByEvaluatingJavaScriptFromString:@"window.outerWidth"] intValue];
    CGFloat scale = webView.frame.size.width / displayWidth;
    CGPoint pt = [sender locationInView:self.webView];
    pt.x *= scale;
    pt.y *= scale;
    pt.x += scrollPositionX;
    pt.y += scrollPositionY;
    NSString *js = [NSString stringWithFormat:@"document.elementFromPoint(%f, %f).tagName", pt.x, pt.y];
    NSString *tagName = [self.webView stringByEvaluatingJavaScriptFromString:js];
    NSString *imgURL = [NSString stringWithFormat:@"document.elementFromPoint(%f, %f).src", pt.x, pt.y];
    NSString *urlToSave = [self.webView stringByEvaluatingJavaScriptFromString:imgURL];
    
    if ([tagName compare:@"IMG" options:NSCaseInsensitiveSearch]==NSOrderedSame) {
        found_img=0;
        
        //NSLog(@"tagName %@",tagName);
        //NSLog(@"urlToSave %@",urlToSave);
        
        if ([[urlToSave pathExtension] compare:@"jpg" options:NSCaseInsensitiveSearch]==NSOrderedSame) found_img=1; //jpg
        if ([[urlToSave pathExtension] compare:@"jpeg" options:NSCaseInsensitiveSearch]==NSOrderedSame) found_img=1; //jpg
        if ([[urlToSave pathExtension] compare:@"png" options:NSCaseInsensitiveSearch]==NSOrderedSame) found_img=2; //png
        if ([[urlToSave pathExtension] compare:@"gif" options:NSCaseInsensitiveSearch]==NSOrderedSame) found_img=3; //gif
        
        if (found_img) {
            UIAlertView *msgAlert;  
            cover_currentPlayFilepath =[detailViewController getCurrentModuleFilepath];
            if (cover_currentPlayFilepath) {
                msgAlert=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Image detected",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Choose_SaveCover",@""),[cover_currentPlayFilepath lastPathComponent]] delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"CoverFolder",@""),NSLocalizedString(@"CoverFile",@""),nil] autorelease];
                
                cover_url_string=[[NSString alloc] initWithString:urlToSave];
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
    
    UITapGestureRecognizer *doubleTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(doubleTap:)];
    doubleTap.numberOfTouchesRequired = 2;
    [self.webView addGestureRecognizer:doubleTap];
    
	self.hidesBottomBarWhenPushed = YES;
	
	webView.scalesPageToFit = YES; 
	webView.autoresizesSubviews = YES; 
	webView.autoresizingMask=(UIViewAutoresizingFlexibleHeight | 
							  UIViewAutoresizingFlexibleWidth); 
	
	[[infoDownloadView layer] setCornerRadius:5.0];	
	[[infoDownloadView layer] setBorderWidth:2.0];
	[[infoDownloadView layer] setBorderColor:[[UIColor colorWithRed: 0.95f green: 0.95f blue: 0.95f alpha: 1.0f] CGColor]];   //Adding Border color.
	infoDownloadView.hidden=YES;
	
	custom_url_count=0;
	[self loadBookmarks];
	[self loadHome];
    [super viewDidLoad];
	
	end_time=clock();	
#ifdef LOAD_PROFILE
	NSLog(@"webbro : %d",end_time-start_time);
#endif
}

/*- (void)viewWillAppear:(BOOL)animated {
 }*/
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
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
	for (int i=0;i<custom_url_count;i++) {
		[custom_URL[i] release];
		[custom_URL_name[i] release];
	}	
    [super dealloc];
}


@end
