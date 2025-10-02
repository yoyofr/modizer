

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#import "RenderViewControllerTVOS.h"
#include "VizController.h"

#include "context.h"
#include "context_metal.h"
#include "keycode_osx.h"
#include "../external/imgui/imgui.h"
#include "../external/imgui/backends/imgui_impl_osx.h"
#include "../CommonApple/CommonApple.h"

@implementation RenderViewControllerTVOS
{
    IBOutlet MTKView *_metal_view;
    //    __weak IBOutlet UIView *_hud_view;
    
    render::metal::IMetalContextPtr _context;
    IVizControllerPtr _vizController;
}


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
//    _vizController->SetControlsVisible(false);
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    @autoreleasepool {
        PROFILE_FRAME()
        
        _context->SetView(view);
        _context->BeginScene();
        _vizController->Render(0, 1);
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




@end

