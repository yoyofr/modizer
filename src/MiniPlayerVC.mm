//
//  MiniPlayerVC.m
//  modizer
//
//  Created by Yohann Magnien on 12/04/2021.
//


int gesture_swipe_min_vel;
int gesture_swipe_min_trans;
int gesture_move_sub_min_trans;
int gesture_move_file_min_trans;
#define SWIPE_MIN_VELOCITY 500
#define SWIPE_MIN_TRANSLATION 50
#define TRIGGER_SUB_MIN_TRANSLATION 100
#define TRIGGER_ENTRY_MIN_TRANSLATION 150
#define TRIGGER_SUB_MIN_WIDTH_RATIO 0.25f
#define TRIGGER_ENTRY_MIN_WIDTH_RATIO 0.5f

#define LABEL_PREVNEXTENTRY_OFFSET 100

#import "ModizerConstants.h"
#import "MiniPlayerVC.h"

#import "DetailViewControllerIphone.h"

@interface MiniPlayerVC ()

@end

@implementation MiniPlayerVC

- (void)viewDidLoad {
    START_PROFILE
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    _mpview=[[MiniPlayerView alloc] init];
    _mpview.translatesAutoresizingMaskIntoConstraints=false;
    [self.view addSubview:_mpview];
    
    // Align leading edge to safe area
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:_mpview attribute:NSLayoutAttributeLeading relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeLeading multiplier:1.0 constant:0]];
    
    // Align trailing edge to safe area
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:_mpview attribute:NSLayoutAttributeTrailing relatedBy:NSLayoutRelationEqual toItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeTrailing multiplier:1.0 constant:0]];
    
    //    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:_mpview attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:_mpview attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    //    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:_mpview attribute:NSLayoutAttributeWidth multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:_mpview attribute:NSLayoutAttributeHeight multiplier:1.0 constant:0]];
    
    _mpview.userInteractionEnabled=true;
    
    UIFont *font = [UIFont systemFontOfSize:12];
    UIFontDescriptor *fontDescriptor = [font fontDescriptor];
    UIFontDescriptor *styleDescriptor = [fontDescriptor fontDescriptorWithSymbolicTraits:[fontDescriptor symbolicTraits]| UIFontDescriptorTraitBold|UIFontDescriptorTraitItalic];
    UIFont *myFont = [UIFont fontWithDescriptor:styleDescriptor size:font.pointSize];
    
    _coverView=[[UIImageView alloc] init];
    [_mpview addSubview:_coverView];
    
    _songInfoView=[[UIView alloc] init];
    _songInfoView.userInteractionEnabled=true;
    _songInfoView.backgroundColor=[UIColor clearColor];
    _songInfoView.clipsToBounds=true;
    [_mpview addSubview:_songInfoView];
    
    _labelPrev=[[UILabel alloc] init];
    _labelPrev.text=NSLocalizedString(@"Previous",@"");
    [_labelPrev setFont:[UIFont italicSystemFontOfSize:12]];
    _labelPrev.textAlignment=NSTextAlignmentCenter;
    [_songInfoView addSubview:_labelPrev];
    
    _labelPrevEntry=[[UILabel alloc] init];
    _labelPrevEntry.text=NSLocalizedString(@"Previous file",@"");
    [_labelPrevEntry setFont:myFont];
    _labelPrevEntry.textAlignment=NSTextAlignmentCenter;
    _labelPrevEntry.alpha=0;
    [_songInfoView addSubview:_labelPrevEntry];
    
    _labelNextEntry=[[UILabel alloc] init];
    _labelNextEntry.text=NSLocalizedString(@"Next file",@"");
    [_labelNextEntry setFont:myFont];
    _labelNextEntry.alpha=0;
    _labelNextEntry.textAlignment=NSTextAlignmentCenter;
    [_songInfoView addSubview:_labelNextEntry];
    
    _labelNext=[[UILabel alloc] init];
    _labelNext.text=NSLocalizedString(@"Next",@"");
    [_labelNext setFont:[UIFont italicSystemFontOfSize:12]];
    _labelNext.textAlignment=NSTextAlignmentCenter;
    [_songInfoView addSubview:_labelNext];
    
    _labelMain=[[CBAutoScrollLabel alloc] init];
    [_labelMain setFont:[UIFont systemFontOfSize:12]];
    _labelMain.labelSpacing = 35; // distance between start and end labels
    _labelMain.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    _labelMain.scrollSpeed = 30; // pixels per second
    _labelMain.textAlignment = NSTextAlignmentLeft; // centers text when no auto-scrolling is applied
    _labelMain.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    _labelMain.userInteractionEnabled=false;
    
    _labelSub=[[CBAutoScrollLabel alloc] init];
    [_labelSub setFont:[UIFont systemFontOfSize:10]];
    _labelSub.labelSpacing = 35; // distance between start and end labels
    _labelSub.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    _labelSub.scrollSpeed = 30; // pixels per second
    _labelSub.textAlignment = NSTextAlignmentLeft; // centers text when no auto-scrolling is applied
    _labelSub.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    
    _labelArtist=[[CBAutoScrollLabel alloc] init];
    [_labelArtist setFont:[UIFont systemFontOfSize:10]];
    _labelArtist.labelSpacing = 35; // distance between start and end labels
    _labelArtist.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    _labelArtist.scrollSpeed = 30; // pixels per second
    _labelArtist.textAlignment = NSTextAlignmentLeft; // centers text when no auto-scrolling is applied
    _labelArtist.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    
    _labelTime=[[UILabel alloc] init];
    [_labelTime setFont:[UIFont systemFontOfSize:11]];
    _labelTime.textAlignment = NSTextAlignmentCenter; // centers text when no auto-scrolling is applied
    _labelTime.userInteractionEnabled=true;
    _labelTime.numberOfLines=0;
    
    _labelPlaylist=[[UILabel alloc] init];
    [_labelPlaylist setFont:[UIFont systemFontOfSize:10]];
    _labelPlaylist.textAlignment = NSTextAlignmentCenter; // centers text when no auto-scrolling is applied
    _labelPlaylist.userInteractionEnabled=true;
    _labelPlaylist.numberOfLines=0;
    
    
    [_songInfoView addSubview:_labelMain];
    [_songInfoView addSubview:_labelSub];
    [_songInfoView addSubview:_labelArtist];
    [_mpview addSubview:_labelTime];
    [_mpview addSubview:_labelPlaylist];
    
    //view to cover cover image + labels and recognize tap + swipe gesture
    _gestureAreaView=[[UIView alloc] init];
    _gestureAreaView.userInteractionEnabled=true;
    _gestureAreaView.translatesAutoresizingMaskIntoConstraints=false;
    [_gestureAreaView setBackgroundColor:[UIColor clearColor]];
    [_mpview addSubview:_gestureAreaView];
    
    // new gesture recognizer
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(pushedGoPlayer)];
    // Set required taps and number of touches
    [tapGesture setNumberOfTapsRequired:1];
    [tapGesture setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [_gestureAreaView addGestureRecognizer:tapGesture];
    
    tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(pushedTime)];
    // Set required taps and number of touches
    [tapGesture setNumberOfTapsRequired:1];
    [tapGesture setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [_labelTime addGestureRecognizer:tapGesture];
    
    if (![[[self parentViewController] title] isEqualToString:NSLocalizedString(@"Now playing",@"")]) {
        //not already on the NowPlaying screen, allow button activtation
        tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(pushedPlaylist)];
        // Set required taps and number of touches
        [tapGesture setNumberOfTapsRequired:1];
        [tapGesture setNumberOfTouchesRequired:1];
        // Add the gesture to the view
        [_labelPlaylist addGestureRecognizer:tapGesture];
    }
    
    UIPanGestureRecognizer *panGesture =[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panLabels:)];
    [_gestureAreaView addGestureRecognizer:panGesture];
    
    [_mpview addConstraint:[NSLayoutConstraint constraintWithItem:_gestureAreaView attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:_mpview attribute:NSLayoutAttributeHeight multiplier:1.0 constant:0]];
    [_mpview addConstraint:[NSLayoutConstraint constraintWithItem:_gestureAreaView attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:_mpview attribute:NSLayoutAttributeTop multiplier:1.0 constant:0]];
    [_mpview addConstraint:[NSLayoutConstraint constraintWithItem:_gestureAreaView attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:_labelMain attribute:NSLayoutAttributeRight multiplier:1.0 constant:0]];
    [_mpview addConstraint:[NSLayoutConstraint constraintWithItem:_gestureAreaView attribute:NSLayoutAttributeLeft relatedBy:NSLayoutRelationEqual toItem:_mpview attribute:NSLayoutAttributeLeft multiplier:1.0 constant:0]];
    
    //Buttons@
    _btnPlay=[[BButton alloc] initWithFrame:CGRectMake(0,1,46,46) type:BButtonTypeGray style:BButtonStyleBootstrapV4];
    [_btnPlay addAwesomeIcon:FAIconPlay beforeTitle:YES];
    [_btnPlay addTarget:self action:@selector(pushedPlay) forControlEvents:UIControlEventTouchUpInside];
    _btnPlay.userInteractionEnabled=true;
    _btnPlay.translatesAutoresizingMaskIntoConstraints = false;
    _btnPlay.hidden=false;
    [_btnPlay setColor:_mpview.backgroundColor];
    [_btnPlay setButtonCornerRadius:[NSNumber numberWithFloat:0.0f]];
    _btnPlay.layer.borderWidth=0;
    [_mpview addSubview:_btnPlay];
    
    _btnPause=[[BButton alloc] initWithFrame:CGRectMake(0,1,46,46) type:BButtonTypeGray style:BButtonStyleBootstrapV4];
    [_btnPause addAwesomeIcon:FAIconPause beforeTitle:YES];
    [_btnPause addTarget:self action:@selector(pushedPause) forControlEvents:UIControlEventTouchUpInside];
    _btnPause.translatesAutoresizingMaskIntoConstraints = false;
    _btnPause.hidden=true;
    [_btnPause setButtonCornerRadius:[NSNumber numberWithFloat:0.0f]];
    [_btnPause setColor:_mpview.backgroundColor];
    _btnPause.layer.borderWidth=0;
    [_mpview addSubview:_btnPause];
    
    NSDictionary *views = NSDictionaryOfVariableBindings(_btnPlay);
    // width constraint
    [_mpview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[_btnPlay(46)]" options:0 metrics:nil views:views]];
    // height constraint
    [_mpview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[_btnPlay(46)]" options:0 metrics:nil views:views]];
    // center align
    [_mpview addConstraint:[NSLayoutConstraint constraintWithItem:_btnPlay attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:_mpview attribute:NSLayoutAttributeRight multiplier:1.0 constant:0]];
    [_mpview addConstraint:[NSLayoutConstraint constraintWithItem:_btnPlay attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:_mpview attribute:NSLayoutAttributeBottom multiplier:1.0 constant:-1]];
    
    views = NSDictionaryOfVariableBindings(_btnPause);
    // width constraint
    [_mpview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[_btnPause(46)]" options:0 metrics:nil views:views]];
    // height constraint
    [_mpview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[_btnPause(46)]" options:0 metrics:nil views:views]];
    // center align
    [_mpview addConstraint:[NSLayoutConstraint constraintWithItem:_btnPause attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:_mpview attribute:NSLayoutAttributeRight multiplier:1.0 constant:0]];
    [_mpview addConstraint:[NSLayoutConstraint constraintWithItem:_btnPause attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:_mpview attribute:NSLayoutAttributeBottom multiplier:1.0 constant:-1]];
    
    
    
    _labelTime_mode=0;
    END_PROFILE
}

- (void)refreshViewDarkmode {
    if (_darkMode) [_mpview setBackgroundColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
    else [_mpview setBackgroundColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
    
    if (_darkMode) {
        _labelPrev.textColor = [UIColor whiteColor];
        _labelNext.textColor = [UIColor whiteColor];
        _labelPrevEntry.textColor = [UIColor whiteColor];
        _labelNextEntry.textColor = [UIColor whiteColor];
        _labelMain.textColor = [UIColor whiteColor];
        _labelSub.textColor = [UIColor whiteColor];
        _labelArtist.textColor = [UIColor whiteColor];
        _labelTime.textColor = [UIColor whiteColor];
        _labelPlaylist.textColor = [UIColor whiteColor];
        //_btnPlay
        [_btnPlay setColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
        [_btnPause setColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
        //_btnPause
    } else {
        _labelPrev.textColor = [UIColor blackColor];
        _labelNext.textColor = [UIColor blackColor];
        _labelPrevEntry.textColor = [UIColor blackColor];
        _labelNextEntry.textColor = [UIColor blackColor];
        _labelMain.textColor = [UIColor blackColor];
        _labelSub.textColor = [UIColor blackColor];
        _labelArtist.textColor = [UIColor blackColor];
        _labelTime.textColor = [UIColor blackColor];
        _labelPlaylist.textColor = [UIColor blackColor];
        [_btnPlay setColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
        [_btnPause setColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
    }
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    _darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) _darkMode=true;
        
    [self refreshViewDarkmode];
}

-(void) pushedGoPlayer {
    _detailVC.view.frame=self.view.frame;
    [self.parentViewController performSelector:@selector(goPlayer)];
}

-(void) swipeRight:(bool)prevFile {
    //[self.parentViewController performSelector:@selector(showWaitingLoading)];
    if (prevFile) [_detailVC playPrev];
    else [_detailVC playPrevSub];
    //[self.parentViewController performSelector:@selector(hideWaiting)];
}

-(void) swipeLeft:(bool)nextFile {
    //[self.parentViewController performSelector:@selector(showWaitingLoading)];
    if (nextFile) [_detailVC playNext];
    else [_detailVC playNextSub];
    //[self.parentViewController performSelector:@selector(hideWaiting)];
}

-(void) pushedPlay {
    [_detailVC playPushed];
}

-(void) pushedPause {
    [_detailVC pausePushed];
}

-(void) pushedTime {
    _labelTime_mode++;
    if (_labelTime_mode>=2) _labelTime_mode=0;
}

-(void) pushedPlaylist {
    [self.parentViewController performSelector:@selector(goCurrentPlaylist)];
}

-(void) panLabels:(UIPanGestureRecognizer*)gesture {
    static CGPoint max_velocity,min_velocity,cur_velocity;
    static int org_centerx,orgPrev_centerx,orgNext_centerx;
    int translationX;
    float alpha,alpha2;
    bool bEntryInsteadOfSub=false;
    CGPoint translation = [gesture translationInView:gesture.view];
    
    
     
    switch (gesture.state) {
        case UIGestureRecognizerStateBegan:
            max_velocity.x=0;
            org_centerx=_labelMain.center.x;
            orgPrev_centerx=_labelPrev.center.x;
            orgNext_centerx=_labelNext.center.x;
            
            gesture_swipe_min_vel=SWIPE_MIN_VELOCITY;
            gesture_swipe_min_trans=SWIPE_MIN_TRANSLATION;
            gesture_move_sub_min_trans=MIN(_gestureAreaView.frame.size.width*TRIGGER_SUB_MIN_WIDTH_RATIO,TRIGGER_SUB_MIN_TRANSLATION);
            gesture_move_file_min_trans=MIN(_gestureAreaView.frame.size.width*TRIGGER_ENTRY_MIN_WIDTH_RATIO,TRIGGER_ENTRY_MIN_TRANSLATION);
            
            break;
        case UIGestureRecognizerStateChanged:
            // move label
            cur_velocity=[gesture velocityInView:gesture.view];
            if (cur_velocity.x>0) {
                if (cur_velocity.x>max_velocity.x) max_velocity.x=cur_velocity.x;
            } else {
                if (cur_velocity.x<max_velocity.x) max_velocity.x=cur_velocity.x;
            }
            
            _labelMain.center = CGPointMake(_labelMain.center.x + translation.x, _labelMain.center.y);
            _labelSub.center = CGPointMake(_labelSub.center.x + translation.x, _labelSub.center.y);
            _labelArtist.center = CGPointMake(_labelArtist.center.x + translation.x, _labelArtist.center.y);
            _labelPrev.center= CGPointMake(_labelPrev.center.x + translation.x, _labelPrev.center.y);
            _labelNext.center= CGPointMake(_labelNext.center.x + translation.x, _labelNext.center.y);
            _labelPrevEntry.center= CGPointMake(_labelPrevEntry.center.x + translation.x, _labelPrevEntry.center.y);
            _labelNextEntry.center= CGPointMake(_labelNextEntry.center.x + translation.x, _labelNextEntry.center.y);
            
            translationX=org_centerx-_labelMain.center.x;
            alpha=(float)translationX*1.0f/(gesture_move_sub_min_trans*2);
            if (alpha<0) alpha=-alpha;
            if (alpha>0.75f) alpha=0.75f;
            
            if ((translationX<-gesture_move_file_min_trans)||(translationX>gesture_move_file_min_trans)) {
                alpha2=1;
            } else {
                alpha2=0;
            }
            
            _labelPrev.alpha=(0.25f+alpha)*(1-alpha2);
            _labelNext.alpha=(0.25f+alpha)*(1-alpha2);
            _labelPrevEntry.alpha=(0.25f+alpha)*alpha2;
            _labelNextEntry.alpha=(0.25f+alpha)*alpha2;
            _labelMain.alpha=(1-alpha);
            _labelSub.alpha=(1-alpha);
            _labelArtist.alpha=(1-alpha);
                        
            break;
        case UIGestureRecognizerStateFailed:
        case UIGestureRecognizerStateCancelled:
        case UIGestureRecognizerStateEnded:
            // reset label
            
            translationX=org_centerx-_labelMain.center.x;
                        
            if (translationX>gesture_move_file_min_trans) bEntryInsteadOfSub=true;
            if (translationX<-gesture_move_file_min_trans) bEntryInsteadOfSub=true;
            
            if (translationX>gesture_move_sub_min_trans) [self swipeLeft:bEntryInsteadOfSub];
            else if (translationX<-gesture_move_sub_min_trans) [self swipeRight:bEntryInsteadOfSub];
            else if ((translationX>gesture_swipe_min_trans)&&(max_velocity.x<-gesture_swipe_min_vel)) [self swipeLeft:bEntryInsteadOfSub];
            else if ((translationX<-gesture_swipe_min_trans)&&(max_velocity.x>gesture_swipe_min_vel)) [self swipeRight:bEntryInsteadOfSub];
            
            [UIView animateWithDuration:0.2 delay:0.0 options:0
                             animations:^{
                self.labelMain.center=CGPointMake(org_centerx,self.labelMain.center.y);
                self.labelSub.center=CGPointMake(org_centerx,self.labelSub.center.y);
                self.labelArtist.center=CGPointMake(org_centerx,self.labelArtist.center.y);
                self.labelPrev.alpha=0;
                self.labelNext.alpha=0;
                self.labelPrevEntry.alpha=0;
                self.labelNextEntry.alpha=0;
                self.labelMain.alpha=1;
                self.labelSub.alpha=1;
                self.labelArtist.alpha=1;
                self.labelPrev.center=CGPointMake(orgPrev_centerx,self.labelPrev.center.y);
                self.labelNext.center=CGPointMake(orgNext_centerx,self.labelNext.center.y);
                self.labelPrevEntry.center=CGPointMake(orgPrev_centerx,self.labelPrevEntry.center.y);
                self.labelNextEntry.center=CGPointMake(orgNext_centerx,self.labelNextEntry.center.y);
                } completion:^(BOOL finished) {
                }];
                        
            break;
    }
            
    // reset translation
    [gesture setTranslation:CGPointZero inView:_labelMain];
}

-(void) refreshTime {
    if (_detailVC.mPlaylist_size==0) {
        self.view.hidden=TRUE;
        return;
    }
    if (self.view.hidden) self.view.hidden=FALSE;
    
    int l=[_detailVC.mplayer getSongLength]/1000;
    int t=[_detailVC.mplayer getCurrentTime]/1000;
    switch (_labelTime_mode) {
        case 0:
            if (l>0) _labelTime.text=[NSString stringWithFormat:@"-%d:%.2d\n-\n%d:%.2d",(l-t)/60,(l-t)%60,l/60,l%60];
            else _labelTime.text=[NSString stringWithFormat:@"--:--"];
            break;
        case 1:
            _labelTime.text=[NSString stringWithFormat:@"%d:%.2d\n-\n%d:%.2d",(t)/60,(t)%60,l/60,l%60];
            break;
    }
    
    if ([_detailVC.mplayer isPlaying]) {
        
        if (![[_detailVC.mplayer getModFileTitle] isEqualToString:_labelMain.text]) [self refreshCoverLabels];
        
        if ([_detailVC.mplayer isPaused]) {
            _btnPlay.hidden=false;
            _btnPause.hidden=true;
        } else {
            _btnPlay.hidden=true;
            _btnPause.hidden=false;
        }
    } else {
        _btnPlay.hidden=false;
        _btnPause.hidden=true;
    }
}

-(void) refreshCoverLabels {
    static bool no_reentrant=false;
    if (no_reentrant) return;
    no_reentrant=true;
    if ([_detailVC.mplayer isPlaying]) {
        
        if ([_detailVC.mplayer isPaused]) {
            _btnPlay.hidden=false;
            _btnPause.hidden=true;
        } else {
            _btnPlay.hidden=true;
            _btnPause.hidden=false;
        }
        _labelMain.text=[_detailVC.mplayer getModFileTitle];
        _labelArtist.text=[_detailVC.mplayer artist];
        if ([_detailVC.mplayer isArchive]&&([_detailVC.mplayer getArcEntriesCnt]>1)) {
            //archive with multiple files
            if (_detailVC.mplayer.mod_subsongs>1) {
                //and also subsongs
                _labelSub.text=[NSString stringWithFormat:@"(%d/%d)(%d/%d) %@",[_detailVC.mplayer getArcIndex]+1,[_detailVC.mplayer getArcEntriesCnt],_detailVC.mplayer.mod_currentsub-_detailVC.mplayer.mod_minsub+1,_detailVC.mplayer.mod_subsongs,[_detailVC.mplayer getModName]];
            } else {
                //no subsong
                _labelSub.text=[NSString stringWithFormat:@"(%d/%d) %@",[_detailVC.mplayer getArcIndex]+1,[_detailVC.mplayer getArcEntriesCnt],[_detailVC.mplayer getModName]];
            }
        } else {
            if (_detailVC.mplayer.mod_subsongs>1) {
                //subsongs
                _labelSub.text=[NSString stringWithFormat:@"(%d/%d) %@",_detailVC.mplayer.mod_currentsub-_detailVC.mplayer.mod_minsub+1,_detailVC.mplayer.mod_subsongs,[_detailVC.mplayer getModName]];
            } else {
                //no subsong
                _labelSub.text=[NSString stringWithFormat:@"%@",[_detailVC.mplayer getModName]];
            }
        }
        _labelPlaylist.text=[NSString stringWithFormat:@"%d\n-\n%d",_detailVC.mPlaylist_pos+1,_detailVC.mPlaylist_size];
        //_labelTime.text=[NSString stringWithFormat:@"%d:%.2d",t/60,t%60];
        [self refreshTime];
    } else {
        _labelMain.text=@"Initializing";
        _labelSub.text=@"...";
        _labelArtist.text=@"";
        _labelTime.text=@"-:--";
                
        if (_detailVC.mPlaylist_size) _labelPlaylist.text=[NSString stringWithFormat:@"%d\n-\n%d",_detailVC.mPlaylist_pos+1,_detailVC.mPlaylist_size];
        else _labelPlaylist.text=@"...\n-\n...";
        
        _btnPause.hidden=true;
        _btnPlay.hidden=false;
    }
    
    [_coverView setImage:_coverImg];
    no_reentrant=false;
}

- (void) refreshCoverView {
    [_coverView setImage:_coverImg];
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];

    // Update layout when _mpview size changes (e.g., when sidebar appears)
    static CGFloat previousWidth = 0;
    CGFloat currentWidth = self.mpview.frame.size.width;

    if (previousWidth != currentWidth && currentWidth > 0) {
        previousWidth = currentWidth;

        // Update frames with new width
        _songInfoView.frame = CGRectMake(50, 0, (currentWidth - 50 - 150), 48);
        _labelMain.frame = CGRectMake(0, 0, (currentWidth - 50 - 150), 24);
        _labelSub.frame = CGRectMake(0, 24, (currentWidth - 50 - 150), 12);
        _labelArtist.frame = CGRectMake(0, 24 + 12, (currentWidth - 50 - 150), 12);

        _labelPrev.frame = CGRectMake(-[_labelPrev.text sizeWithAttributes:@{NSFontAttributeName:_labelPrev.font}].width, 0, [_labelPrev.text sizeWithAttributes:@{NSFontAttributeName:_labelPrev.font}].width, 48);
        _labelPrevEntry.frame = CGRectMake(-[_labelPrevEntry.text sizeWithAttributes:@{NSFontAttributeName:_labelPrevEntry.font}].width, 0, [_labelPrevEntry.text sizeWithAttributes:@{NSFontAttributeName:_labelPrevEntry.font}].width, 48);
        _labelNextEntry.frame = CGRectMake((currentWidth - 50 - 150), 0, [_labelNextEntry.text sizeWithAttributes:@{NSFontAttributeName:_labelNextEntry.font}].width, 48);
        _labelNext.frame = CGRectMake((currentWidth - 50 - 150), 0, [_labelNext.text sizeWithAttributes:@{NSFontAttributeName:_labelNext.font}].width, 48);

        _labelTime.frame = CGRectMake(currentWidth - 100, 0, 50, 48);
        _labelPlaylist.frame = CGRectMake(currentWidth - 150, 0, 50, 48);
    }

    // Update gesture parameters
    gesture_swipe_min_vel = SWIPE_MIN_VELOCITY;
    gesture_swipe_min_trans = SWIPE_MIN_TRANSLATION;
    gesture_move_sub_min_trans = MIN(_gestureAreaView.frame.size.width * TRIGGER_SUB_MIN_WIDTH_RATIO, TRIGGER_SUB_MIN_TRANSLATION);
    gesture_move_file_min_trans = MIN(_gestureAreaView.frame.size.width * TRIGGER_ENTRY_MIN_WIDTH_RATIO, TRIGGER_ENTRY_MIN_TRANSLATION);
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    float ww=self.mpview.frame.size.width;
    
    _songInfoView.frame=CGRectMake(50,0,(ww-50-150),48);
    _labelMain.frame=CGRectMake(0,0,(ww-50-150),24);
    _labelSub.frame=CGRectMake(0,24,(ww-50-150),12);
    _labelArtist.frame=CGRectMake(0,24+12,(ww-50-150),12);
        
    _labelPrev.frame=CGRectMake(-[_labelPrev.text sizeWithAttributes:@{NSFontAttributeName:_labelPrev.font}].width,0,[_labelPrev.text sizeWithAttributes:@{NSFontAttributeName:_labelPrev.font}].width,48);
    _labelPrevEntry.frame=CGRectMake(-[_labelPrevEntry.text sizeWithAttributes:@{NSFontAttributeName:_labelPrevEntry.font}].width,0,[_labelPrevEntry.text sizeWithAttributes:@{NSFontAttributeName:_labelPrevEntry.font}].width,48);
    _labelNextEntry.frame=CGRectMake((ww-50-150),0,[_labelNextEntry.text sizeWithAttributes:@{NSFontAttributeName:_labelNextEntry.font}].width,48);
    _labelNext.frame=CGRectMake((ww-50-150),0,[_labelNext.text sizeWithAttributes:@{NSFontAttributeName:_labelNext.font}].width,48);
    
    _labelTime.frame=CGRectMake(ww-100,0,50,48);
    _labelPlaylist.frame=CGRectMake(ww-150,0,50,48);
    
    
    [self refreshCoverLabels];
}


- (UIViewController *)visibleViewController:(UIViewController *)rootViewController
{
    if ([rootViewController isKindOfClass:[UITabBarController class]])
    {
        UIViewController *selectedViewController = ((UITabBarController *)rootViewController).selectedViewController;
        
        return [self visibleViewController:selectedViewController];
    }
    if ([rootViewController isKindOfClass:[UINavigationController class]])
    {
        UIViewController *lastViewController = [[((UINavigationController *)rootViewController) viewControllers] lastObject];
        
        return [self visibleViewController:lastViewController];
    }
    
    if (rootViewController.presentedViewController == nil)
    {
        return rootViewController;
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UINavigationController class]])
    {
        UINavigationController *navigationController = (UINavigationController *)rootViewController.presentedViewController;
        UIViewController *lastViewController = [[navigationController viewControllers] lastObject];
        
        return [self visibleViewController:lastViewController];
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UITabBarController class]])
    {
        UITabBarController *tabBarController = (UITabBarController *)rootViewController.presentedViewController;
        UIViewController *selectedViewController = tabBarController.selectedViewController;
        
        return [self visibleViewController:selectedViewController];
    }
    
    UIViewController *presentedViewController = (UIViewController *)rootViewController.presentedViewController;
    
    return [self visibleViewController:presentedViewController];
}


- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    
    
    //issue when checking minivc directly, so check visible viewcontroller instead
    UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
    _darkMode=false;
    if (vc.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) _darkMode=true;
    
    //Background color
    
    if (_darkMode) [_mpview setBackgroundColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
    else [_mpview setBackgroundColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
    
    //Cover artwork
    _coverView.frame=CGRectMake(0,0,48,48);
    
    
    //Labels
    float ww=self.mpview.frame.size.width;
    
    //if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
//    if (@available(iOS 14.0, *)) {
//        if ([NSProcessInfo processInfo].isiOSAppOnMac) {
//            UIScreen* mainscr = [UIScreen mainScreen];
//            UIWindow *win=[UIApplication sharedApplication].keyWindow;
//            
//            ww=win.bounds.size.width;
//        }
//    } else {
//        
//    }
    
    _songInfoView.frame=CGRectMake(50,0,(ww-50-150),48);
    
    _labelPrev.frame=CGRectMake(-[_labelPrev.text sizeWithAttributes:@{NSFontAttributeName:_labelPrev.font}].width,0,[_labelPrev.text sizeWithAttributes:@{NSFontAttributeName:_labelPrev.font}].width,48);
    
    if (_darkMode) _labelPrev.textColor = [UIColor whiteColor];
    else _labelPrev.textColor = [UIColor blackColor];
    
    _labelPrevEntry.frame=CGRectMake(-[_labelPrevEntry.text sizeWithAttributes:@{NSFontAttributeName:_labelPrevEntry.font}].width,0,[_labelPrevEntry.text sizeWithAttributes:@{NSFontAttributeName:_labelPrevEntry.font}].width,48);
    if (_darkMode) _labelPrevEntry.textColor = [UIColor whiteColor];
    else _labelPrevEntry.textColor = [UIColor blackColor];

    _labelNextEntry.frame=CGRectMake((ww-50-150),0,[_labelNextEntry.text sizeWithAttributes:@{NSFontAttributeName:_labelNextEntry.font}].width,48);
    
    if (_darkMode) _labelNextEntry.textColor = [UIColor whiteColor];
    else _labelNextEntry.textColor = [UIColor blackColor];
    
    _labelNext.frame=CGRectMake((ww-50-150),0,[_labelNext.text sizeWithAttributes:@{NSFontAttributeName:_labelNext.font}].width,48);
    if (_darkMode) _labelNext.textColor = [UIColor whiteColor];
    else _labelNext.textColor = [UIColor blackColor];
    
    _labelMain.frame=CGRectMake(0,0,(ww-50-150),24);
    if (_darkMode) _labelMain.textColor = [UIColor whiteColor];
    else _labelMain.textColor = [UIColor blackColor];
    
    _labelSub.frame=CGRectMake(0,24,(ww-50-150),12);
    if (_darkMode) _labelSub.textColor = [UIColor whiteColor];
    else _labelSub.textColor = [UIColor blackColor];
    
    _labelArtist.frame=CGRectMake(0,24+12,(ww-50-150),12);
    if (_darkMode) _labelArtist.textColor = [UIColor whiteColor];
    else _labelArtist.textColor = [UIColor blackColor];
    
    _labelTime.frame=CGRectMake(ww-100,0,50,48);
    if (_darkMode) _labelTime.textColor = [UIColor whiteColor];
    else _labelTime.textColor = [UIColor blackColor];
    
    _labelPlaylist.frame=CGRectMake(ww-150,0,50,48);
    if (_darkMode) _labelPlaylist.textColor = [UIColor whiteColor];
    else _labelPlaylist.textColor = [UIColor blackColor];
    [self refreshCoverLabels];
    
    [_btnPlay setColor:_mpview.backgroundColor];
    
    [_btnPause setColor:_mpview.backgroundColor];

    //Timer
    _repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.20f target:self selector:@selector(refreshTime) userInfo:nil repeats: YES]; //5 times/second
    
    if ([_detailVC.mplayer isPlaying]) {
        if ([_detailVC.mplayer isPaused]) {
            _btnPlay.hidden=false;
            _btnPause.hidden=true;
        } else {
            _btnPlay.hidden=true;
            _btnPause.hidden=false;
        }
    }
}

-(void)viewDidDisappear:(BOOL)animated {
    [_repeatingTimer invalidate];
    _repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
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
