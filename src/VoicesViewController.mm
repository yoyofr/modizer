//
//  VoicesViewController.m
//  modizer
//
//  Created by Yohann Magnien on 02/04/2021.
//

#import "VoicesViewController.h"

@interface VoicesViewController ()

@end

@implementation VoicesViewController

@synthesize detailViewController;
@synthesize scrollView;

-(IBAction) goPlayer {
    [self.navigationController pushViewController:detailViewController animated:YES];
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}


- (void)viewDidLoad {
    currentPlayingFile=NULL;
    
    for (int i=0;i<SOUND_MAXMOD_CHANNELS;i++) {
        voicesLbl[i]=NULL;
        voices[i]=NULL;
        voicesSolo[i]=NULL;
    }
    voicesAllOn=NULL;
    voicesAllOff=NULL;
    
    NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.1 target:self selector:@selector(checkPlayer) userInfo:nil repeats:YES];
    
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
}

- (void) viewWillDisappear:(BOOL)animated {
    [self removeVoicesButtons];
    
    [super viewWillDisappear:animated];
}


- (void) viewWillAppear:(BOOL)animated {
    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    
    [self resetVoicesButtons];
    [self recomputeFrames];
    
    [super viewWillAppear:animated];
}

/*- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
}*/

- (void)viewDidLayoutSubviews {
    [self recomputeFrames];    
}


/*- (BOOL)shouldAutorotate {
    [self shouldAutorotateToInterfaceOrientation:self.interfaceOrientation];
    return TRUE;
}*/

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    [self recomputeFrames];
    [self.view setNeedsDisplay];
    return YES;
}


/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/



-(void)pushedVoicesChanged:(id)sender {
    if ([detailViewController.mplayer isPlaying]&&([currentPlayingFile compare:[detailViewController.mplayer mod_currentfile]]==NSOrderedSame)) {
        for (int i=0;i<SOUND_MAXMOD_CHANNELS;i++) {
            if (voices[i]==sender) {
                if ([detailViewController.mplayer getVoicesStatus:i]) {
                    [voices[i] setType:BButtonTypeInverse];
                    [voices[i] setTitle:[NSString stringWithFormat:@"%C",FAIconVolumeOff] forState:UIControlStateNormal];
                    [detailViewController.mplayer setVoicesStatus:0 index:i];
                } else {
                    [voices[i] setType:BButtonTypePrimary];
                    [voices[i] setTitle:[NSString stringWithFormat:@"%C",FAIconVolumeUp] forState:UIControlStateNormal];
                    [detailViewController.mplayer setVoicesStatus:1 index:i];
                }
            }
        }
    } else {
        [self resetVoicesButtons];
        [self recomputeFrames];
    }
}

-(void)pushedSoloVoice:(id)sender {
    if ([detailViewController.mplayer isPlaying]&&([currentPlayingFile compare:[detailViewController.mplayer mod_currentfile]]==NSOrderedSame)) {
        for (int i=0;i<detailViewController.mplayer.numChannels;i++) {
            if (voicesSolo[i]==sender) {
                [voices[i] setType:BButtonTypePrimary];
                [voices[i] setTitle:[NSString stringWithFormat:@"%C",FAIconVolumeUp] forState:UIControlStateNormal];
                [detailViewController.mplayer setVoicesStatus:1 index:i];
            } else {
                [voices[i] setType:BButtonTypeInverse];
                [voices[i] setTitle:[NSString stringWithFormat:@"%C",FAIconVolumeOff] forState:UIControlStateNormal];
                [detailViewController.mplayer setVoicesStatus:0 index:i];
            }
        }
    } else {
        [self resetVoicesButtons];
        [self recomputeFrames];
    }
}

-(void)pushedAllVoicesOn:(id)sender {
    if ([detailViewController.mplayer isPlaying]&&([currentPlayingFile compare:[detailViewController.mplayer mod_currentfile]]==NSOrderedSame)) {
        for (int i=0;i<detailViewController.mplayer.numChannels;i++) {
            [voices[i] setType:BButtonTypePrimary];            
            [voices[i] setTitle:[NSString stringWithFormat:@"%C",FAIconVolumeUp] forState:UIControlStateNormal];
            [detailViewController.mplayer setVoicesStatus:1 index:i];
        }
    } else {
        [self resetVoicesButtons];
        [self recomputeFrames];
    }
}

-(void)pushedAllVoicesOff:(id)sender {
    if ([detailViewController.mplayer isPlaying]&&([currentPlayingFile compare:[detailViewController.mplayer mod_currentfile]]==NSOrderedSame)) {
        for (int i=0;i<detailViewController.mplayer.numChannels;i++) {
            [voices[i] setType:BButtonTypeInverse];
            [voices[i] setTitle:[NSString stringWithFormat:@"%C",FAIconVolumeOff] forState:UIControlStateNormal];
            [detailViewController.mplayer setVoicesStatus:0 index:i];
        }
    } else {
        [self resetVoicesButtons];
        [self recomputeFrames];
    }
}

-(void) checkPlayer {
    if ([detailViewController.mplayer isPlaying]&&([currentPlayingFile compare:[detailViewController.mplayer mod_currentfile]]==NSOrderedSame)) {
    } else {
        [self resetVoicesButtons];
        [self recomputeFrames];
    }
}



- (void) recomputeFrames {
    static bool no_reentrant=false;
    if (no_reentrant) return;
    no_reentrant=true;
    if ([detailViewController.mplayer isVoicesMutingSupported]&&detailViewController.mplayer.numChannels) {
        int rows_nb=self.scrollView.frame.size.width/(128+16+32);
        int rows_width=self.scrollView.frame.size.width/rows_nb;
        int ypos=4;
        int xpos=(self.scrollView.frame.size.width-rows_nb*rows_width)/2;
        
        voicesAllLbl.frame=CGRectMake(xpos,ypos,96,32);
        voicesAllOn.frame=CGRectMake(xpos+96+8,ypos,32,32);
        voicesAllOff.frame=CGRectMake(xpos+128+8,ypos,32,32);
        
        ypos+=40;
        for (int i=0;i<detailViewController.mplayer.numChannels;i++) {
            if (voicesLbl[i]) voicesLbl[i].frame=CGRectMake(xpos,ypos,96,32);
            if (voicesSolo[i]) voicesSolo[i].frame=CGRectMake(xpos+96+8,ypos,32,32);
            if (voices[i]) voices[i].frame=CGRectMake(xpos+128+16,ypos,32,32);
            
            if ((i+1)%rows_nb) {
                xpos+=rows_width;
            } else {
                xpos=(self.scrollView.frame.size.width-rows_nb*rows_width)/2;;
                ypos+=40;
            }
            
        }
        self.scrollView.contentSize = CGSizeMake(self.view.frame.size.width, ypos);
    }
    no_reentrant=false;
}


- (void) removeVoicesButtons {
    for (int i=0;i<SOUND_MAXMOD_CHANNELS;i++) {
        if (voicesLbl[i]) [voicesLbl[i] removeFromSuperview];
        if (voices[i]) [voices[i] removeFromSuperview];
        if (voicesSolo[i]) [voicesSolo[i] removeFromSuperview];
        voicesSolo[i]=NULL;
        voicesLbl[i]=NULL;
        voices[i]=NULL;
    }
    if (voicesAllOn) [voicesAllOn removeFromSuperview];
    if (voicesAllOff) [voicesAllOff removeFromSuperview];
    if (voicesAllLbl) [voicesAllLbl removeFromSuperview];
    voicesAllOn=NULL;
    voicesAllOff=NULL;
}

- (void) resetVoicesButtons {
    [self removeVoicesButtons];
    if ([detailViewController.mplayer isPlaying]) {
        currentPlayingFile=[detailViewController.mplayer mod_currentfile];
        if ([detailViewController.mplayer isVoicesMutingSupported]&&detailViewController.mplayer.numChannels) {
            int ypos=4;
            int xpos=4;
            
            voicesAllOn=[[BButton alloc] initWithFrame:CGRectMake(0,0,32,32) type:BButtonTypePrimary style:BButtonStyleBootstrapV2];
            [voicesAllOn addTarget:self action:@selector(pushedAllVoicesOn:) forControlEvents:UIControlEventTouchUpInside];
            [voicesAllOn addAwesomeIcon:FAIconVolumeUp beforeTitle:true];
            [self.scrollView addSubview:voicesAllOn];
            
            voicesAllOff=[[BButton alloc] initWithFrame:CGRectMake(0,0,32,32) type:BButtonTypeInverse style:BButtonStyleBootstrapV2];
            [voicesAllOff addTarget:self action:@selector(pushedAllVoicesOff:) forControlEvents:UIControlEventTouchUpInside];
            //[voicesAllOff setTitle:@"Off" forState:UIControlStateNormal];
            [voicesAllOff addAwesomeIcon:FAIconVolumeOff beforeTitle:true];
            [self.scrollView addSubview:voicesAllOff];
            
            voicesAllLbl=[[UILabel alloc] initWithFrame:CGRectMake(0,0,96,20)];
            voicesAllLbl.font=[UIFont boldSystemFontOfSize:16];
            voicesAllLbl.text=NSLocalizedString(@"All voices",@"All voices");
            voicesAllLbl.backgroundColor=[UIColor clearColor];
            voicesAllLbl.textColor=[UIColor whiteColor];
            [self.scrollView addSubview:voicesAllLbl];
            
            
            for (int i=0;i<detailViewController.mplayer.numChannels;i++) {
                voicesLbl[i]=[[UILabel alloc] initWithFrame:CGRectMake(0,0,96,20)];
                voicesLbl[i].font=[UIFont boldSystemFontOfSize:12];
                voicesLbl[i].text=[detailViewController.mplayer getVoicesName:i];
                voicesLbl[i].backgroundColor=[UIColor clearColor];
                voicesLbl[i].textColor=[UIColor whiteColor];

                voices[i]=[[BButton alloc] initWithFrame:CGRectMake(0,0,32,32) type:([detailViewController.mplayer getVoicesStatus:i]?BButtonTypePrimary:BButtonTypeInverse) style:BButtonStyleBootstrapV2];
                [voices[i] addAwesomeIcon:([detailViewController.mplayer getVoicesStatus:i]?FAIconVolumeUp:FAIconVolumeOff) beforeTitle:true];
                [voices[i] addTarget:self action:@selector(pushedVoicesChanged:) forControlEvents:UIControlEventTouchUpInside];
                
                voicesSolo[i]=[[BButton alloc] initWithFrame:CGRectMake(0,0,32,32) type:BButtonTypePrimary style:BButtonStyleBootstrapV2];
                [voicesSolo[i] addTarget:self action:@selector(pushedSoloVoice:) forControlEvents:UIControlEventTouchUpInside];
                [voicesSolo[i] setTitle:@"S" forState:UIControlStateNormal];
                [self.scrollView addSubview:voicesSolo[i]];
                
                [self.scrollView addSubview:voicesLbl[i]];
                [self.scrollView addSubview:voices[i]];
            }
        }
    }
}


@end
