//
//  MiniPlayerVC.m
//  modizer
//
//  Created by Yohann Magnien on 12/04/2021.
//

#import "ModizerConstants.h"
#import "MiniPlayerVC.h"

#import "DetailViewControllerIphone.h"

@interface MiniPlayerVC ()

@end

@implementation MiniPlayerVC

@synthesize detailVC;
@synthesize coverImg;
@synthesize mpview;

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    mpview=[[MiniPlayerView alloc] init];
    mpview.translatesAutoresizingMaskIntoConstraints=false;
    [self.view addSubview:mpview];
    
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeWidth multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeHeight multiplier:1.0 constant:0]];
    
    mpview.userInteractionEnabled=true;
    
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
    if (darkMode) [mpview setBackgroundColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
    else [mpview setBackgroundColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
    
    if (darkMode) {
        labelMain.textColor = [UIColor lightGrayColor];
        labelSub.textColor = [UIColor lightGrayColor];
    } else {
        labelMain.textColor = [UIColor darkGrayColor];
        labelSub.textColor = [UIColor darkGrayColor];
    }
}

-(void) pushedGoPlayer {
    [self.parentViewController performSelector:@selector(goPlayer)];
}

-(void) swipeRight {
    [detailVC playPrevSub];
}

-(void) swipeLeft {
    [detailVC playNextSub];
}

-(void) refreshCoverLabels {
    if ([detailVC.mplayer isPlaying]) {
    labelMain.text=[detailVC.mplayer getModFileTitle];
    if ([detailVC.mplayer isArchive]&&([detailVC.mplayer getArcEntriesCnt]>1)) {
        //archive with multiple files
        if (detailVC.mplayer.mod_subsongs>1) {
            //and also subsongs
            labelSub.text=[NSString stringWithFormat:@"(%d/%d)(%d/%d) %@",[detailVC.mplayer getArcIndex],[detailVC.mplayer getArcEntriesCnt],detailVC.mplayer.mod_currentsub,detailVC.mplayer.mod_maxsub-detailVC.mplayer.mod_minsub,[detailVC.mplayer getModName]];
        } else {
            //no subsong
            labelSub.text=[NSString stringWithFormat:@"(%d/%d) %@",[detailVC.mplayer getArcIndex],[detailVC.mplayer getArcEntriesCnt],[detailVC.mplayer getModName]];
        }
    } else {
        if (detailVC.mplayer.mod_subsongs>1) {
            //subsongs
            labelSub.text=[NSString stringWithFormat:@"(%d/%d) %@",detailVC.mplayer.mod_currentsub,detailVC.mplayer.mod_maxsub-detailVC.mplayer.mod_minsub,[detailVC.mplayer getModName]];
        } else {
            //no subsong
            labelSub.text=[NSString stringWithFormat:@"%@",[detailVC.mplayer getModName]];
        }
    }
    } else {
        labelMain.text=@"Initializing";
        labelSub.text=@"...";
    }
    
    [coverView setImage:coverImg];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    labelMain.frame=CGRectMake(50,0,(size.width-50-100),24);
    labelSub.frame=CGRectMake(50,24,(size.width-50-100),24);
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
        
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
    //Background color
    if (darkMode) [mpview setBackgroundColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
    else [mpview setBackgroundColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
    
    //Cover artwork
    coverView=[[UIImageView alloc] init];
    coverView.frame=CGRectMake(0,0,48,48);
    [mpview addSubview:coverView];
    
    
    //Labels
    int ww=self.view.frame.size.width;
    labelMain=[[CBAutoScrollLabel alloc] init];
    labelMain.frame=CGRectMake(50,0,(ww-50-100),24);
    [labelMain setFont:[UIFont systemFontOfSize:12]];
    if (darkMode) labelMain.textColor = [UIColor whiteColor];
    else labelMain.textColor = [UIColor blackColor];
    labelMain.labelSpacing = 35; // distance between start and end labels
    labelMain.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    labelMain.scrollSpeed = 30; // pixels per second
    labelMain.textAlignment = NSTextAlignmentLeft; // centers text when no auto-scrolling is applied
    labelMain.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    labelMain.userInteractionEnabled=true;
    //[labelMain setBackgroundColor:[UIColor redColor]];
    
    labelSub=[[CBAutoScrollLabel alloc] init];
    labelSub.frame=CGRectMake(50,24,(ww-50-100),24);
    [labelSub setFont:[UIFont systemFontOfSize:10]];
    if (darkMode) labelSub.textColor = [UIColor whiteColor];
    else labelSub.textColor = [UIColor blackColor];
    labelSub.labelSpacing = 35; // distance between start and end labels
    labelSub.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    labelSub.scrollSpeed = 30; // pixels per second
    labelSub.textAlignment = NSTextAlignmentLeft; // centers text when no auto-scrolling is applied
    labelSub.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    labelSub.userInteractionEnabled=true;
    
    [self refreshCoverLabels];
    
    
    [mpview addSubview:labelMain];
    [mpview addSubview:labelSub];
    
    //view to cover cover image + labels and recognize tap + swipe gesture
    gestureAreaView=[[UIView alloc] init];
    gestureAreaView.userInteractionEnabled=true;
    [gestureAreaView setBackgroundColor:[UIColor clearColor]];
    [mpview addSubview:gestureAreaView];
    // new gesture recognizer
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(pushedGoPlayer)];
    // Set required taps and number of touches
    [tapGesture setNumberOfTapsRequired:1];
    [tapGesture setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [gestureAreaView addGestureRecognizer:tapGesture];
    
    // new gesture recognizer
    UISwipeGestureRecognizer *swipeGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipeLeft)];
    // Set required direction and number of touches
    [swipeGesture setDirection:UISwipeGestureRecognizerDirectionLeft];
    [swipeGesture setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [gestureAreaView addGestureRecognizer:swipeGesture];
    
    swipeGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipeRight)];
    // Set required direction and number of touches
    [swipeGesture setDirection:UISwipeGestureRecognizerDirectionRight];
    [swipeGesture setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [gestureAreaView addGestureRecognizer:swipeGesture];
    gestureAreaView.translatesAutoresizingMaskIntoConstraints=false;
    
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:gestureAreaView attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeHeight multiplier:1.0 constant:0]];
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:gestureAreaView attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeWidth multiplier:1.0 constant:-100]];
    //Buttons@
}

-(void)viewDidDisappear:(BOOL)animated {
    [coverView removeFromSuperview];coverView=NULL;
    [labelMain removeFromSuperview];labelMain=NULL;
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
