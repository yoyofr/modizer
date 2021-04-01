//
//  EQViewController.m
//  modizer
//
//  Created by Yohann Magnien on 07/08/13.
//
//

#import "EQViewController.h"
#import "QuartzCore/CAGradientLayer.h"

/*
 */

//NVDSP
#import "Novocaine.h"
#import "NVDSP.h"
#import "NVPeakingEQFilter.h"
#import "NVSoundLevelMeter.h"
#import "NVClippingDetection.h"

extern NVPeakingEQFilter *nvdsp_PEQ[10];
extern BOOL nvdsp_EQ;


@interface EQViewController ()

@end

@implementation EQViewController

@synthesize detailViewController;

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

- (void)sliderEQGainChanged:(id)sender {
    UISlider *slider=(UISlider *)sender;
    float delta=slider.value-eqGlobalGainLastValue;
    eqGlobalGainLastValue=slider.value;
    for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        nvdsp_PEQ[i].G+=delta;
        if (nvdsp_PEQ[i].G>12) nvdsp_PEQ[i].G=12;
        if (nvdsp_PEQ[i].G<-12) nvdsp_PEQ[i].G=-12;
        slider=(UISlider*)[self.view viewWithTag:i+1];
        slider.value=nvdsp_PEQ[i].G;
    }
}

-(void)switchEQChanged:(id)sender {
    nvdsp_EQ=((UISwitch*)sender).on;
}

-(void)pushedVoicesChanged:(id)sender {
    if ([detailViewController.mplayer isPlaying]&&([currentPlayingFile compare:[detailViewController.mplayer mod_currentfile]]==NSOrderedSame)) {
        for (int i=0;i<SOUND_MAXMOD_CHANNELS;i++) {
            if (voices[i]==sender) {
                if ([detailViewController.mplayer getVoicesStatus:i]) {
                    [voices[i] setType:BButtonTypeInverse];
                    [detailViewController.mplayer setVoicesStatus:0 index:i];
                } else {
                    [voices[i] setType:BButtonTypePrimary];
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
                [detailViewController.mplayer setVoicesStatus:1 index:i];
            } else {
                [voices[i] setType:BButtonTypeInverse];
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


- (void)sliderChanged:(id)sender {
    UISlider *slider=(UISlider *)sender;
    nvdsp_PEQ[slider.tag-1].G=slider.value;
    eqLabelValue[slider.tag-1].text=[NSString stringWithFormat:@"%.1fdB",nvdsp_PEQ[slider.tag-1].G];
}

+ (void) restoreEQSettings {
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    NSString *str;
    
    for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        str=[NSString stringWithFormat:@"eq_centerFrequencies_%d",i];
        valNb=[prefs objectForKey:str];
        if (valNb!=nil) nvdsp_PEQ[i].centerFrequency=[valNb floatValue];
        
        str=[NSString stringWithFormat:@"eq_Q_%d",i];
        valNb=[prefs objectForKey:str];
        if (valNb!=nil) nvdsp_PEQ[i].Q=[valNb floatValue];
        
        str=[NSString stringWithFormat:@"eq_G_%d",i];
        valNb=[prefs objectForKey:str];
        if (valNb!=nil) nvdsp_PEQ[i].G=[valNb floatValue];
    }
    
    valNb=[prefs objectForKey:@"nvdsp_EQ"];
    if (valNb!=nil) nvdsp_EQ=[valNb boolValue];
}

+ (void) backupEQSettings {
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    NSString *str;
    
	for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        str=[NSString stringWithFormat:@"eq_centerFrequencies_%d",i];
        valNb=[[NSNumber alloc] initWithFloat:nvdsp_PEQ[i].centerFrequency];
        [prefs setObject:valNb forKey:str];
    
        str=[NSString stringWithFormat:@"eq_Q_%d",i];
        valNb=[[NSNumber alloc] initWithFloat:nvdsp_PEQ[i].Q];
        [prefs setObject:valNb forKey:str];
    
        str=[NSString stringWithFormat:@"eq_G_%d",i];
        valNb=[[NSNumber alloc] initWithFloat:nvdsp_PEQ[i].G];
        [prefs setObject:valNb forKey:str];
    }
    
    valNb=[[NSNumber alloc] initWithBool:nvdsp_EQ];
    [prefs setObject:valNb forKey:@"nvdsp_EQ"];
    
    [prefs synchronize];
}



- (void)viewDidLoad
{
    [super viewDidLoad];
    
    currentPlayingFile=NULL;
    
    for (int i=0;i<SOUND_MAXMOD_CHANNELS;i++) {
        voicesLbl[i]=NULL;
        voices[i]=NULL;
        voicesSolo[i]=NULL;
    }
    voicesAllOn=NULL;
    voicesAllOff=NULL;
    
    CGAffineTransform sliderRotation = CGAffineTransformIdentity;
    sliderRotation = CGAffineTransformRotate(sliderRotation, -(M_PI / 2));
    
    for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        eqSlider[i]=[[UISlider alloc] init];
        [eqSlider[i] addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
        
        [eqSlider[i] setThumbImage:[UIImage imageNamed:@"slider.png" ] forState:UIControlStateNormal];

        
        eqSlider[i].tag=i+1;
        eqSlider[i].maximumValue=12;
        eqSlider[i].minimumValue=-12;
        eqSlider[i].transform=sliderRotation;
        eqSlider[i].frame=CGRectMake(10+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),32,16,self.view.frame.size.height/4);
        [self.view addSubview:eqSlider[i]];
        //[eqSlider[i] release];
        
        eqLabelFreq[i]=[[UILabel alloc] initWithFrame:CGRectMake(8+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),16,32,16)];
        eqLabelFreq[i].font=[UIFont boldSystemFontOfSize:8];
        eqLabelFreq[i].backgroundColor=[UIColor clearColor];
        eqLabelFreq[i].textColor=[UIColor whiteColor];
        
        [self.view addSubview:eqLabelFreq[i]];
        //[eqLabelFreq[i] release];
        
        eqLabelValue[i]=[[UILabel alloc] initWithFrame:CGRectMake(8+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),self.view.frame.size.height/4+32,32,16)];
        eqLabelValue[i].font=[UIFont systemFontOfSize:7];
        eqLabelValue[i].backgroundColor=[UIColor clearColor];
        eqLabelValue[i].textColor=[UIColor whiteColor];
        
        [self.view addSubview:eqLabelValue[i]];
        //[eqLabelValue[i] release];
    }
    
    eqGlobalGain=[[UISlider alloc] init];
    [eqGlobalGain addTarget:self action:@selector(sliderEQGainChanged:) forControlEvents:UIControlEventValueChanged];
    
    [eqGlobalGain setThumbImage:[UIImage imageNamed:@"slider.png" ] forState:UIControlStateNormal];
    
    eqGlobalGain.tag=100;
    eqGlobalGain.maximumValue=12;
    eqGlobalGain.minimumValue=-12;
    eqGlobalGain.value=0;
    eqGlobalGainLastValue=0;
    eqGlobalGain.transform=sliderRotation;
    eqGlobalGain.frame=CGRectMake(10+(self.view.frame.size.width-34),32,16,self.view.frame.size.height/4);
    [self.view addSubview:eqGlobalGain];
    //[eqGlobalGain release];
    
    minus12DB=[[UILabel alloc] initWithFrame:CGRectMake(4,self.view.frame.size.height/4+32,28,20)];
    minus12DB.font=[UIFont boldSystemFontOfSize:8];
    minus12DB.text=@"-12dB";
    minus12DB.backgroundColor=[UIColor clearColor];
    minus12DB.textColor=[UIColor whiteColor];
    
    [self.view addSubview:minus12DB];
    //[minus12DB release];
    
    plus12DB=[[UILabel alloc] initWithFrame:CGRectMake(4,32,28,20)];
    plus12DB.font=[UIFont boldSystemFontOfSize:8];
    plus12DB.text=@"+12dB";
    plus12DB.backgroundColor=[UIColor clearColor];
    plus12DB.textColor=[UIColor whiteColor];
    
    [self.view addSubview:plus12DB];
    //[plus12DB release];
    
    zeroDB=[[UILabel alloc] initWithFrame:CGRectMake(4,self.view.frame.size.height/4+32-10,28,20)];
    zeroDB.font=[UIFont boldSystemFontOfSize:8];
    zeroDB.text=@"+0dB";
    zeroDB.backgroundColor=[UIColor clearColor];
    zeroDB.textColor=[UIColor whiteColor];
    
    [self.view addSubview:zeroDB];
    //[zeroDB release];
    
    globalGain=[[UILabel alloc] initWithFrame:CGRectMake(10+(self.view.frame.size.width-34),self.view.frame.size.height/4+32,32,16)];
    globalGain.font=[UIFont systemFontOfSize:8];
    globalGain.text=@"Gbl";
    globalGain.backgroundColor=[UIColor clearColor];
    globalGain.textColor=[UIColor whiteColor];
    [self.view addSubview:globalGain];
    //[globalGain release];
    
    eqOnOff=[[UISwitch alloc] init];
    eqOnOff.frame=CGRectMake(10,self.view.frame.size.height/4+32,32,32);
    [eqOnOff addTarget:self action:@selector(switchEQChanged:) forControlEvents:UIControlEventValueChanged];
    eqOnOff.on=nvdsp_EQ;
    [self.view addSubview:eqOnOff];
    //[eqOnOff release];
    
    eqOnOffLbl=[[UILabel alloc] initWithFrame:CGRectMake(4,self.view.frame.size.height/4+64,100,20)];
    eqOnOffLbl.font=[UIFont boldSystemFontOfSize:10];
    eqOnOffLbl.text=@"Apply equalizer";
    eqOnOffLbl.backgroundColor=[UIColor clearColor];
    eqOnOffLbl.textColor=[UIColor whiteColor];
    
    [self.view addSubview:eqOnOffLbl];
    //[eqOnOffLbl release];
    
    NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.1 target:self selector:@selector(checkPlayer) userInfo:nil repeats:YES];
    
}

- (void) recomputeFrames {
    
    if ([detailViewController.mplayer isVoicesMutingSupported]&&detailViewController.mplayer.numChannels) {
        int ypos=self.view.frame.size.height/4+64+40;
        int xpos=4;
        
        voicesAllOn.frame=CGRectMake(self.view.frame.size.width-80,ypos-40,32,32);
        voicesAllOff.frame=CGRectMake(self.view.frame.size.width-40,ypos-40,32,32);
        for (int i=0;i<detailViewController.mplayer.numChannels;i++) {
            if (voicesLbl[i]) voicesLbl[i].frame=CGRectMake(xpos,ypos,96,32);
            if (voices[i]) voices[i].frame=CGRectMake(xpos+128,ypos,32,32);
            
            voicesSolo[i].frame=CGRectMake(xpos+96,ypos,32,32);
            
            if (i&1) {
                xpos=4;
                ypos+=40;
            } else xpos+=self.view.frame.size.width/2;
            
        }
    }
    
    for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        eqSlider[i].frame=CGRectMake(10+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),32,16,self.view.frame.size.height/4);
        eqLabelFreq[i].frame=CGRectMake(8+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),16,32,16);
        eqLabelValue[i].frame=CGRectMake(8+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),self.view.frame.size.height/4+32,32,16);
    }
    minus12DB.frame=CGRectMake(4,self.view.frame.size.height/4+32-23,28,20);
    plus12DB.frame=CGRectMake(4,32+1,28,20);
    zeroDB.frame=CGRectMake(4,self.view.frame.size.height/4+32-11,28,20);
    
    globalGain.frame=CGRectMake(10+(self.view.frame.size.width-34),self.view.frame.size.height/4+32,32,16);
    eqGlobalGain.frame=CGRectMake(10+(self.view.frame.size.width-34),32,16,self.view.frame.size.height/4);
    
    eqOnOff.frame=CGRectMake(80+10,self.view.frame.size.height/4+64,32,20);

    eqOnOffLbl.frame=CGRectMake(4,self.view.frame.size.height/4+64+2,80,20);
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
    voicesAllOn=NULL;
    voicesAllOff=NULL;
}

- (void) resetVoicesButtons {
    [self removeVoicesButtons];
    if ([detailViewController.mplayer isPlaying]) {
        currentPlayingFile=[detailViewController.mplayer mod_currentfile];
        if ([detailViewController.mplayer isVoicesMutingSupported]&&detailViewController.mplayer.numChannels) {
            int ypos=self.view.frame.size.height/4+64+32;
            int xpos=4;
            
            voicesAllOn=[[BButton alloc] initWithFrame:CGRectMake(self.view.frame.size.width-64,ypos,32,32)];
            [voicesAllOn setType:BButtonTypePrimary];
            [voicesAllOn addTarget:self action:@selector(pushedAllVoicesOn:) forControlEvents:UIControlEventTouchUpInside];
            [voicesAllOn setTitle:@"On" forState:UIControlStateNormal];
            [self.view addSubview:voicesAllOn];
            
            voicesAllOff=[[BButton alloc] initWithFrame:CGRectMake(self.view.frame.size.width-32,ypos,32,32)];
            [voicesAllOff setType:BButtonTypePrimary];
            [voicesAllOff addTarget:self action:@selector(pushedAllVoicesOff:) forControlEvents:UIControlEventTouchUpInside];
            [voicesAllOff setTitle:@"Off" forState:UIControlStateNormal];
            [self.view addSubview:voicesAllOff];
            
            for (int i=0;i<detailViewController.mplayer.numChannels;i++) {
                voicesLbl[i]=[[UILabel alloc] initWithFrame:CGRectMake(xpos,ypos,96,20)];
                voicesLbl[i].font=[UIFont boldSystemFontOfSize:12];
                voicesLbl[i].text=[detailViewController.mplayer getVoicesName:i];
                voicesLbl[i].backgroundColor=[UIColor clearColor];
                voicesLbl[i].textColor=[UIColor whiteColor];

                voices[i]=[[BButton alloc] initWithFrame:CGRectMake(xpos+128,ypos,32,32)];
                [voices[i] setType:([detailViewController.mplayer getVoicesStatus:i]?BButtonTypePrimary:BButtonTypeInverse)];
                [voices[i] setTitle:@" " forState:UIControlStateNormal];
                [voices[i] addTarget:self action:@selector(pushedVoicesChanged:) forControlEvents:UIControlEventTouchUpInside];
                
                voicesSolo[i]=[[BButton alloc] initWithFrame:CGRectMake(xpos+96,ypos,32,32)];
                [voicesSolo[i] setType:BButtonTypePrimary];
                [voicesSolo[i] addTarget:self action:@selector(pushedSoloVoice:) forControlEvents:UIControlEventTouchUpInside];
                [voicesSolo[i] setTitle:@"S" forState:UIControlStateNormal];
                [self.view addSubview:voicesSolo[i]];
                
                //[btnShowSubSong setType:BButtonTypeInverse];
                //[btnShowArcList addAwesomeIcon:0x187 beforeTitle:YES font_size:20];
                
                if (i&1) {
                    xpos=4;
                    ypos+=32;
                } else xpos+=self.view.frame.size.width/2;
                
                [self.view addSubview:voicesLbl[i]];
                [self.view addSubview:voices[i]];
            }
        }
    }
}

- (void) viewWillDisappear:(BOOL)animated {
    [EQViewController backupEQSettings];
    
    [self removeVoicesButtons];
    
    [super viewWillDisappear:animated];
}


- (void) viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    
    [self resetVoicesButtons];
    
    [self recomputeFrames];
    
    for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        eqSlider[i].value=nvdsp_PEQ[i].G;
        if (nvdsp_PEQ[i].centerFrequency>=1000) {
            eqLabelFreq[i].text=[NSString stringWithFormat:@"%.0fKhz",nvdsp_PEQ[i].centerFrequency/1000];
        } else {
            eqLabelFreq[i].text=[NSString stringWithFormat:@"%.0fHz",nvdsp_PEQ[i].centerFrequency];
        }
        eqLabelValue[i].text=[NSString stringWithFormat:@"%.1fdB",nvdsp_PEQ[i].G];
        
    }
}

- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
}

- (BOOL)shouldAutorotate {
    [self shouldAutorotateToInterfaceOrientation:self.interfaceOrientation];
    return TRUE;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    [self recomputeFrames];
    [self.view setNeedsDisplay];
	return YES;
}

- (void)dealloc {
    //[super dealloc];
}


- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
