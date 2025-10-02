

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <GLKit/GLKit.h>

#import "RenderViewControllerIOS.h"
#include "VizController.h"
#include "audio/IAudioSource.h"

#include "context.h"
#include "context_metal.h"
#include "keycode_osx.h"
#include "../external/imgui/imgui.h"
#include "../external/imgui/backends/imgui_impl_osx.h"
#include "../CommonApple/CommonApple.h"

#import "ExternalViewController.h"

struct ToolbarItemInfo
{
    UIToolbarButton button;
    NSString * imageName = nil;
};

static ToolbarItemInfo _toolbar_iteminfo[] =
{
    {UIToolbarButton::FlexibleSpace},
    {UIToolbarButton::NavigatePrevious, @"arrow.left.circle"},
    {UIToolbarButton::NavigateNext,     @"arrow.right.circle"},
    
    {UIToolbarButton::FixedSpace},
    
    {UIToolbarButton::SelectionLock,    @"lock.open.fill"},
    {UIToolbarButton::SelectionUnlock,  @"lock.fill"},
    {UIToolbarButton::NavigateShuffle,  @"shuffle"},

    {UIToolbarButton::FixedSpace},
    
    {UIToolbarButton::Play,             @"play.fill"},
    {UIToolbarButton::Pause,            @"pause.fill"},
    {UIToolbarButton::Step,             @"forward.frame.fill"},

    {UIToolbarButton::FixedSpace},
    
    {UIToolbarButton::Like,             @"heart"},
    {UIToolbarButton::Unlike,           @"heart.fill"},
    {UIToolbarButton::Blacklist,        @"heart.slash"},
    {UIToolbarButton::Unblacklist,      @"heart.slash.fill"},

    {UIToolbarButton::FixedSpace},
    
    {UIToolbarButton::MicrophoneOn,  @"mic.fill"},
    {UIToolbarButton::MicrophoneOff, @"mic.slash.fill"},
    
    {UIToolbarButton::FixedSpace},
    
    {UIToolbarButton::SettingsShow,      @"gearshape"},
    {UIToolbarButton::SettingsHide,     @"gearshape.fill"},
    
    {UIToolbarButton::FlexibleSpace},
};
    

@implementation RenderViewControllerIOS
{
    __weak IBOutlet MTKView *_metal_view;
    __weak IBOutlet UIToolbar *_toolbar_view;
    __weak IBOutlet UIView *_hud_view;
    __weak IBOutlet UIButton *_hud_preset_title;
    
    std::vector<UIBarButtonItem *> _toolbaritem_list;
    std::vector<bool> _toolbaritem_visible;
    
    UIWindow * _externalWindow;
    ExternalViewController * _externalWindowController;
    render::metal::IMetalContextPtr _context;
    IVizControllerPtr _vizController;
    
    std::string _preset_title;
    std::string _old_preset_title;
    
}

#if !TARGET_OS_TV

-(BOOL)prefersStatusBarHidden
{
    return YES;
}


-(BOOL)shouldAutorotate
{
    return YES;
}

#endif

-(BOOL)prefersHomeIndicatorAutoHidden
{
    return YES;
}

- (void)viewDidLoad
{
    PROFILE_FUNCTION()

    [super viewDidLoad];
    
    [self initMetal];
    
    std::string resourceDir;
    GetResourceDir(resourceDir);
    std::string assetDir =  PathCombine(resourceDir, "assets");
    std::string userDir;
    GetApplicationSupportDir(userDir);


    _vizController = CreateVizController(_context, assetDir, userDir);
      

    [self setupToolbar];
    [self setupScreens];
    [self setupGestures];
    
    _hud_preset_title.layer.cornerRadius = 8;
    _hud_preset_title.layer.borderColor = [UIColor darkGrayColor].CGColor;
    _hud_preset_title.layer.borderWidth = 1.0f;
    [_hud_preset_title.layer setMasksToBounds:YES];
    
    _vizController->HideDebugUI();
}

-(void)initMetal
{
    PROFILE_FUNCTION()

    id <MTLDevice> device = MTLCreateSystemDefaultDevice();
    if(!device)
    {
        NSLog(@"Metal is not supported on this device");
        return;
    }
    MTKView *view = _metal_view;
    view.device = device;
    
    // configure view
//    view.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;

#if 0
    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
#else
    
#if TARGET_OS_SIMULATOR
    view.colorPixelFormat = MTLPixelFormatRGBA16Float;
//    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
//    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
//    view.colorPixelFormat = MTLPixelFormatBGR10_XR;

#else
    view.colorPixelFormat = MTLPixelFormatBGR10_XR;
//    view.colorPixelFormat = MTLPixelFormatRGB9E5Float;
    
//        view.colorPixelFormat = MTLPixelFormatBGR10_XR;
//    view.colorPixelFormat = MTLPixelFormatRGBA16Float;

#endif
    

#endif
    
    view.depthStencilPixelFormat = MTLPixelFormatInvalid;
    view.sampleCount = 1;
    view.preferredFramesPerSecond = 60;
    view.delegate = self;
//    view.eventDelegate = self;
//    [view registerDragDrop];
    

//    self.view = view;
    
    
    


    if (@available(tvOS 13.0, iOS 13.0, *))
    {
    
        CAMetalLayer *layer = (CAMetalLayer *)view.layer;
//        layer.wantsExtendedDynamicRangeContent= YES;
//        layer.pixelFormat = view.colorPixelFormat;
        const CFStringRef name = kCGColorSpaceExtendedSRGB;
//        const CFStringRef name = kCGColorSpaceExtendedLinearSRGB;
    //    const CFStringRef name = kCGColorSpaceSRGB;
    //    const CFStringRef name = kCGColorSpaceSRGB;

        CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(name);
        layer.colorspace = colorspace;
        CGColorSpaceRelease(colorspace);
    }




    
    

    _context = render::metal::MetalCreateContext(device);
    _context->SetView( view );
    _context->SetRenderTarget(nullptr);
    
}



-(void)updateUI
{
    _vizController->SetControlsVisible(false);
    
    if (!_vizController->IsDebugUIVisible())
    {
        // UI is hidden
        _hud_view.hidden = YES;
        return;
    }
    

    // UI is visible
    _hud_view.hidden = NO;
    
    // update preset title
    _vizController->GetPresetTitle(_preset_title);
    if (_preset_title != _old_preset_title)
    {
        [_hud_preset_title  setTitle: [NSString stringWithUTF8String:_preset_title.c_str()]
                            forState:UIControlStateNormal
         ];
        _old_preset_title = _preset_title;
    }
    
    // update toolbar
    [self updateToolbar];
}

-(void)updateToolbar
{
    if (!_vizController)
        return;
    
    size_t count = _toolbaritem_list.size();

    bool updateToolbar = false;
    int itemCount  = 0;
    for (size_t i=0; i < count; i++)
    {
        UIBarButtonItem *item = _toolbaritem_list[i];
        UIToolbarButton buttonid = (UIToolbarButton)(item.tag);
        bool visible = _vizController->IsToolbarButtonVisible( buttonid );
        
        // did visibilty state change?
        if (visible != _toolbaritem_visible[i])
        {
            _toolbaritem_visible[i] = visible;
            updateToolbar = true;
        }
        
        if (visible)
        {
            itemCount++;
        }
    }
    
    
    if (updateToolbar)
    {
        // rebuild toolbar items....
        NSMutableArray<UIBarButtonItem *>  *items = [NSMutableArray<UIBarButtonItem *> arrayWithCapacity:itemCount];
       
        for (size_t i=0; i < _toolbaritem_list.size(); i++)
        {
            if (_toolbaritem_visible[i])
            {
                [items addObject: _toolbaritem_list[i]];
            }
        }
        
        _toolbar_view.items = items;
    }

}


-(void)onToolbarButtonPressed:(UIBarButtonItem *)sender
{
    UIToolbarButton button =  (UIToolbarButton)sender.tag;

    // trigger button press...
    if (_vizController)
        _vizController->OnToolbarButtonPressed(button);
    
    [self updateToolbar];
}

-(void)setupToolbar
{
    // create all button and space  items
    int count = sizeof(_toolbar_iteminfo) /  sizeof(_toolbar_iteminfo[0]);

    _toolbaritem_list.reserve(count);
    _toolbaritem_visible.reserve(count);
    
    for (int i=0; i < count; i++)
    {
        const auto &info = _toolbar_iteminfo[i];
        
        if (info.button == UIToolbarButton::FlexibleSpace) {
            UIBarButtonItem *space_item = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace
                                                                                  target:nil
                                                                                  action:nil];
            space_item.tag = (NSInteger)info.button;
            _toolbaritem_list.push_back(space_item);
        } else  if (info.button == UIToolbarButton::FixedSpace) {
            UIBarButtonItem *space_item = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFixedSpace
                                                                                  target:nil
                                                                                  action:nil];
            space_item.width = 40.0f;
            space_item.tag = (NSInteger)info.button;
            _toolbaritem_list.push_back(space_item);
        } else {
            
            NSString  *imageName = info.imageName;
            
            
            UIImage *image = [UIImage systemImageNamed:imageName];
            UIBarButtonItem *button =  [[UIBarButtonItem alloc]
                                        initWithImage:image
                                            style:UIBarButtonItemStylePlain
                                            target:self
                                            action:@selector(onToolbarButtonPressed:)
             ];
            
            button.width = 40.0f;
            button.tag = (NSInteger)info.button;
            _toolbaritem_list.push_back(button);
        }
        
        _toolbaritem_visible.push_back(false);

    }
    
    [self updateToolbar];
}

-(void)setupScreens
{
    auto screens = [UIScreen screens];
    if (screens.count > 1) {
        [self setupExternalScreen:screens[1]];
    }
    
    
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self selector:@selector(handleScreenConnectNotification:) name:UIScreenDidConnectNotification object:nil];
    [center addObserver:self selector:@selector(handleScreenDisconnectNotification:) name:UIScreenDidDisconnectNotification object:nil];
    

}



- (void)handleScreenConnectNotification:(NSNotification*)notification {
    NSLog(@"Screen connected!");    
    UIScreen *screen = (UIScreen *)notification.object;
    [self setupExternalScreen:screen];
}

- (void)handleScreenDisconnectNotification:(NSNotification*)notification {
    NSLog(@"Screen disconnected");
    UIScreen *screen = (UIScreen *)notification.object;
    [self shutdownExternalScreen:screen];
}


-(void)setupExternalScreen:(UIScreen *)screen
{
    if (!_context)
        return;
    
    if (_externalWindow)
        return;
    
    UIStoryboard *sb = self.storyboard;
    _externalWindowController = (ExternalViewController *)[sb  instantiateViewControllerWithIdentifier:@"ExternalWindowScene"];
    _externalWindowController.context = _context;
    _externalWindowController.visualizer = _vizController;

    
    _externalWindow = [[UIWindow alloc] initWithFrame:screen.bounds];
    _externalWindow.screen = screen;
    _externalWindow.rootViewController = _externalWindowController;
    _externalWindow.hidden = NO;

    NSLog(@"UIWindow: %@ screen:%fx%f window:%fx%f\n", _externalWindow,
          screen.bounds.size.width, screen.bounds.size.height,
             _externalWindow.bounds.size.width,
             _externalWindow.bounds.size.height
             );


}

-(void)shutdownExternalScreen:(UIScreen *)screen
{
    if (!_externalWindow)
        return;
    
    if (_externalWindow.screen != screen)
        return;

    _externalWindowController = nil;
    _externalWindow.hidden = YES;
    _externalWindow = nil;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void)respondToTapGesture:(UIGestureRecognizer *)gestureRecognizer
{
//    NSLog(@"tap!\n");
    
//    vizController->ToggleDebugMenu();
    
//    vizController->NavigateForward();
}

-(void)respondToDoubleTapGesture:(UIGestureRecognizer *)gestureRecognizer
{
//    _vizController->NavigateForward();
}


-(void)respondToSwipeLeftGesture:(UIGestureRecognizer *)gestureRecognizer
{
//    _vizController->NavigateHistoryPrevious();
}



-(void)respondToSwipeRightGesture:(UIGestureRecognizer *)gestureRecognizer
{
//    _vizController->NavigateHistoryNext();
}


-(void)setupGestures
{
#if 0
    {
        // Create and initialize a tap gesture
        UITapGestureRecognizer *tapRecognizer = [[UITapGestureRecognizer alloc]
                                                 initWithTarget:self
                                                 action:@selector(respondToTapGesture:)];
        
    #if TARGET_OS_TV
        tapRecognizer.allowedPressTypes = @[[NSNumber numberWithInteger:UIPressTypeSelect], [NSNumber numberWithInteger:UIPressTypePlayPause]];
    #endif
        
        // Specify that the gesture must be a single tap
        tapRecognizer.numberOfTapsRequired = 1;
        
        // Add the tap gesture recognizer to the view
        [self.view addGestureRecognizer:tapRecognizer];
    }
#endif

#if 0
    {
           // Create and initialize a tap gesture
           UITapGestureRecognizer *r = [[UITapGestureRecognizer alloc]
                                                    initWithTarget:self
                                                    action:@selector(respondToDoubleTapGesture:)];
           
      
           
           // Specify that the gesture must be a single tap
           r.numberOfTapsRequired = 2;
           
           // Add the tap gesture recognizer to the view
           [self.view addGestureRecognizer:r];
       }
#endif
    
    
#if TARGET_OS_IOS && 0

    {
        UISwipeGestureRecognizer *r = [[UISwipeGestureRecognizer alloc]
                                                       initWithTarget:self
                                                       action:@selector(respondToSwipeLeftGesture:)];
        r.numberOfTouchesRequired = 2;
        r.direction = UISwipeGestureRecognizerDirectionLeft;
        [self.view addGestureRecognizer:r];
    }
    
    {
        UISwipeGestureRecognizer *r = [[UISwipeGestureRecognizer alloc]
                                                       initWithTarget:self
                                                       action:@selector(respondToSwipeRightGesture:)];
        r.numberOfTouchesRequired = 2;
        r.direction = UISwipeGestureRecognizerDirectionRight;
        [self.view addGestureRecognizer:r];
    }
#endif
    
    
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    @autoreleasepool {
        PROFILE_FRAME()

        int screenCount = 1;
        
        if (_externalWindow != nil)
        {
            // force enable debug UI if external window is up
            _vizController->ShowDebugUI();
            screenCount = 2;
        }
        
        _context->SetView(view);
        _context->BeginScene();
        _vizController->Render(0, screenCount);
        _context->EndScene();
        _context->Present();
        
        
        // release touches
        ImGuiIO &io = ImGui::GetIO();
        if (!io.MouseDown[0])
        {
            io.MousePos = ImVec2(FLT_MAX, FLT_MAX);
        }

        
        [self updateUI];

    }
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
//    NSLog(@"drawableSizeWillChange: %@ %fx%f\n", view, size.width, size.height);

}


#if TARGET_OS_IOS

// This touch mapping is super cheesy/hacky. We treat any touch on the screen
// as if it were a depressed left mouse button, and we don't bother handling
// multitouch correctly at all. This causes the "cursor" to behave very erratically
// when there are multiple active touches. But for demo purposes, single-touch
// interaction actually works surprisingly well.
- (void)updateIOWithTouchEvent:(UIEvent *)event {

    ImGuiIO &io = ImGui::GetIO();

    UITouch *anyTouch = event.allTouches.anyObject;
    if (!anyTouch) return;
    
    if (anyTouch.view != _hud_view && anyTouch.view != _metal_view )
    {
        // not within any imgui view...
        io.MousePos = ImVec2(FLT_MAX, FLT_MAX);
        io.MouseDown[0] = false;
        return;
    }
        
    CGPoint touchLocation = [anyTouch locationInView:anyTouch.view];
    io.MousePos = ImVec2(touchLocation.x, touchLocation.y);
    
    BOOL hasActiveTouch = NO;
    for (UITouch *touch in event.allTouches) {
        if (touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled) {
            hasActiveTouch = YES;
            break;
        }
    }
    io.MouseDown[0] = hasActiveTouch;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

#endif





@end

