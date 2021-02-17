//
// VidStabViewController.m
//
// Copyright (c) 2019 Taner Sener
//
// This file is part of MobileFFmpeg.
//
// MobileFFmpeg is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MobileFFmpeg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
//

#import <AVFoundation/AVFoundation.h>
#import <AVKit/AVKit.h>
#import <mobileffmpeg/MobileFFmpegConfig.h>
#import <mobileffmpeg/MobileFFmpeg.h>
#import "VidStabViewController.h"

@interface VidStabViewController ()

@property (strong, nonatomic) IBOutlet UILabel *header;
@property (strong, nonatomic) IBOutlet UIButton *stabilizeVideoButton;
@property (strong, nonatomic) IBOutlet UILabel *videoPlayerFrame;
@property (strong, nonatomic) IBOutlet UILabel *stabilizedVideoPlayerFrame;

@end

@implementation VidStabViewController {

    // Video player references
    AVQueuePlayer *player;
    AVPlayerLayer *playerLayer;
    AVQueuePlayer *stabilizedVideoPlayer;
    AVPlayerLayer *stabilizedVideoPlayerLayer;

    // Loading view
    UIActivityIndicatorView* indicator;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // STYLE UPDATE
    [Util applyButtonStyle: self.stabilizeVideoButton];
    [Util applyVideoPlayerFrameStyle: self.videoPlayerFrame];
    [Util applyVideoPlayerFrameStyle: self.stabilizedVideoPlayerFrame];
    [Util applyHeaderStyle: self.header];

    // VIDEO PLAYER INIT
    player = [[AVQueuePlayer alloc] init];
    playerLayer = [AVPlayerLayer playerLayerWithPlayer:player];
    playerLayer.videoGravity = AVLayerVideoGravityResize;
    stabilizedVideoPlayer = [[AVQueuePlayer alloc] init];
    stabilizedVideoPlayerLayer = [AVPlayerLayer playerLayerWithPlayer:stabilizedVideoPlayer];
    stabilizedVideoPlayerLayer.videoGravity = AVLayerVideoGravityResize;

    // SETTING VIDEO FRAME POSITIONS
    CGRect upperRectangularFrame = CGRectMake(self.videoPlayerFrame.frame.origin.x + 20,
                                         self.videoPlayerFrame.frame.origin.y + 20,
                                         self.videoPlayerFrame.frame.size.width - 40,
                                         self.videoPlayerFrame.frame.size.height - 40);

    playerLayer.frame = upperRectangularFrame;
    [self.view.layer addSublayer:playerLayer];

    CGRect lowerRectangularFrame = CGRectMake(self.stabilizedVideoPlayerFrame.frame.origin.x + 20,
                                              self.stabilizedVideoPlayerFrame.frame.origin.y + 20,
                                              self.stabilizedVideoPlayerFrame.frame.size.width - 40,
                                              self.stabilizedVideoPlayerFrame.frame.size.height - 40);

    stabilizedVideoPlayerLayer.frame = lowerRectangularFrame;
    [self.view.layer addSublayer:stabilizedVideoPlayerLayer];

    dispatch_async(dispatch_get_main_queue(), ^{
        [self setActive];
    });
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)logCallback:(long)executionId :(int)level :(NSString*)message {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"%@", message);
    });
}

- (IBAction)stabilizedVideoClicked:(id)sender {
    NSString *resourceFolder = [[NSBundle mainBundle] resourcePath];
    NSString *image1 = [resourceFolder stringByAppendingPathComponent: @"colosseum.jpg"];
    NSString *image2 = [resourceFolder stringByAppendingPathComponent: @"pyramid.jpg"];
    NSString *image3 = [resourceFolder stringByAppendingPathComponent: @"tajmahal.jpg"];
    NSString *shakeResultsFile = [self getShakeResultsFilePath];
    NSString *videoFile = [self getVideoPath];
    NSString *stabilizedVideoFile = [self getStabilizedVideoPath];
    
    if (player != nil) {
        [player removeAllItems];
    }
    if (stabilizedVideoPlayer != nil) {
        [stabilizedVideoPlayer removeAllItems];
    }

    [[NSFileManager defaultManager] removeItemAtPath:shakeResultsFile error:NULL];
    [[NSFileManager defaultManager] removeItemAtPath:videoFile error:NULL];
    [[NSFileManager defaultManager] removeItemAtPath:stabilizedVideoFile error:NULL];
    
    NSLog(@"Testing VID.STAB\n");
    
    [self loadProgressDialog:@"Creating video\n\n"];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        
        NSString* ffmpegCommand = [VidStabViewController generateVideoCreateScript:image1:image2:image3:videoFile];
        
        NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", ffmpegCommand);
        
        // EXECUTE
        int result = [MobileFFmpeg execute: ffmpegCommand];
        
        NSLog(@"FFmpeg process exited with rc %d\n", result);
        
        dispatch_async(dispatch_get_main_queue(), ^{
            if (result == RETURN_CODE_SUCCESS) {
                [self dismissProgressDialog];
                
                NSLog(@"Create completed successfully; stabilizing video.\n");
                
                NSString *analyzeVideoCommand = [NSString stringWithFormat:@"-hide_banner -y -i %@ -vf vidstabdetect=shakiness=10:accuracy=15:result=%@ -f null -", videoFile, shakeResultsFile];
                
                [self loadProgressDialog:@"Stabilizing video\n\n"];
                
                dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                    
                    NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", analyzeVideoCommand);
                    
                    // EXECUTE
                    int result = [MobileFFmpeg execute: analyzeVideoCommand];
                    
                    NSLog(@"FFmpeg process exited with rc %d\n", result);
                    
                    dispatch_async(dispatch_get_main_queue(), ^{
                        if (result == RETURN_CODE_SUCCESS) {

                            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                                
                                NSString *stabilizeVideoCommand = [NSString stringWithFormat:@"-hide_banner -y -i %@ -vf vidstabtransform=smoothing=30:input=%@ %@", videoFile, shakeResultsFile, stabilizedVideoFile];
                                
                                NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", stabilizeVideoCommand);
                                
                                // EXECUTE
                                int result = [MobileFFmpeg execute: stabilizeVideoCommand];
                                
                                NSLog(@"FFmpeg process exited with rc %d\n", result);
                                
                                dispatch_async(dispatch_get_main_queue(), ^{
                                    if (result == RETURN_CODE_SUCCESS) {
                                        [self dismissProgressDialog];
                                        
                                        NSLog(@"Stabilize video completed successfully; playing videos.\n");
                                        [self playVideo];
                                        [self playStabilizedVideo];
                                    } else {
                                        NSLog(@"Stabilize video failed with rc=%d\n", result);
                                        
                                        [self dismissProgressDialogAndAlert:@"Stabilize video failed. Please check log for the details."];
                                    }
                                });
                            });

                        } else {
                            NSLog(@"Stabilize video failed with rc=%d\n", result);

                            dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, 1.3 * NSEC_PER_SEC);
                            dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
                                [self dismissProgressDialogAndAlert:@"Stabilize video failed. Please check log for the details."];
                            });
                        }
                    });
                });
                
            } else {
                NSLog(@"Create failed with rc=%d\n", result);
                
                [self dismissProgressDialogAndAlert:@"Create video failed. Please check log for the details."];
            }
        });
    });
}

- (void)playVideo {
    NSString *videoFile = [self getVideoPath];
    NSURL*videoURL=[NSURL fileURLWithPath:videoFile];
    AVAsset *asset = [AVAsset assetWithURL:videoURL];
    NSArray *assetKeys = @[@"playable", @"hasProtectedContent"];
    AVPlayerItem *video = [AVPlayerItem playerItemWithAsset:asset
                                         automaticallyLoadedAssetKeys:assetKeys];
   
    [player insertItem:video afterItem:nil];
    [player play];
}

- (void)playStabilizedVideo {
    NSString *stabilizedVideoFile = [self getStabilizedVideoPath];
    NSURL*stabilizedVideoURL=[NSURL fileURLWithPath:stabilizedVideoFile];
    AVAsset *asset = [AVAsset assetWithURL:stabilizedVideoURL];
    NSArray *assetKeys = @[@"playable", @"hasProtectedContent"];
    AVPlayerItem *stabilizedVideo = [AVPlayerItem playerItemWithAsset:asset
                                  automaticallyLoadedAssetKeys:assetKeys];
    
    [stabilizedVideoPlayer insertItem:stabilizedVideo afterItem:nil];
    [stabilizedVideoPlayer play];
}

- (NSString*)getShakeResultsFilePath {
    NSString* docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    return [docFolder stringByAppendingPathComponent: @"transforms.trf"];
}

- (NSString*)getVideoPath {
    NSString* docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    return [docFolder stringByAppendingPathComponent: @"video.mp4"];
}

- (NSString*)getStabilizedVideoPath {
    NSString* docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    return [docFolder stringByAppendingPathComponent: @"video-stabilized.mp4"];
}

- (void)loadProgressDialog:(NSString*) dialogMessage {
    UIAlertController *pending = [UIAlertController alertControllerWithTitle:nil
                                                                     message:dialogMessage
                                                              preferredStyle:UIAlertControllerStyleAlert];
    indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
    indicator.color = [UIColor blackColor];
    indicator.translatesAutoresizingMaskIntoConstraints=NO;
    [pending.view addSubview:indicator];
    NSDictionary * views = @{@"pending" : pending.view, @"indicator" : indicator};
    
    NSArray * constraintsVertical = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[indicator]-(20)-|" options:0 metrics:nil views:views];
    NSArray * constraintsHorizontal = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|[indicator]|" options:0 metrics:nil views:views];
    NSArray * constraints = [constraintsVertical arrayByAddingObjectsFromArray:constraintsHorizontal];
    [pending.view addConstraints:constraints];
    [indicator startAnimating];
    [self presentViewController:pending animated:YES completion:nil];
}

- (void)dismissProgressDialog {
    [indicator stopAnimating];
    [self dismissViewControllerAnimated:TRUE completion:nil];
}

- (void)dismissProgressDialogAndAlert: (NSString*)message {
    [indicator stopAnimating];
    [self dismissViewControllerAnimated:TRUE completion:^{
        [Util alert:self withTitle:@"Error" message:message andButtonText:@"OK"];
    }];
}

- (void)setActive {
    [MobileFFmpegConfig setLogDelegate:self];
}

+ (NSString*)generateVideoCreateScript:(NSString *)image1 :(NSString *)image2 :(NSString *)image3 :(NSString *)videoFile {
    return [NSString stringWithFormat:
@"-hide_banner -y -loop 1 -i %@ \
-loop 1 -i %@ \
-loop 1 -i %@ \
-f lavfi -i color=black:s=640x427 \
-filter_complex \"\
[0:v]setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1[stream1out];\
[1:v]setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1[stream2out];\
[2:v]setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1[stream3out];\
[stream1out]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=3[stream1overlaid];\
[stream2out]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=3[stream2overlaid];\
[stream3out]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=3[stream3overlaid];\
[3:v][stream1overlaid]overlay=x=\'2*mod(n,4)\':y=\'2*mod(n,2)\',trim=duration=3[stream1shaking];\
[3:v][stream2overlaid]overlay=x=\'2*mod(n,4)\':y=\'2*mod(n,2)\',trim=duration=3[stream2shaking];\
[3:v][stream3overlaid]overlay=x=\'2*mod(n,4)\':y=\'2*mod(n,2)\',trim=duration=3[stream3shaking];\
[stream1shaking][stream2shaking][stream3shaking]concat=n=3:v=1:a=0,scale=w=640:h=424,format=yuv420p[video]\" \
-map [video] -vsync 2 -async 1 -c:v mpeg4 -r 30 %@", image1, image2, image3, videoFile];
}

@end
