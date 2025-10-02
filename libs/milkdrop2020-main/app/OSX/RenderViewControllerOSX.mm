

#import <GLKit/GLKit.h>
#include <sys/stat.h>
#include <sys/types.h>

#import "RenderViewControllerOSX.h"

#include "IVizController.h"
#include "VizController.h"
#include "render/context.h"
#include "render/context_metal.h"
#include "render/context_gles.h"
#include "keycode_osx.h"
#include "../external/imgui/imgui.h"
#include "../external/imgui/backends/imgui_impl_osx.h"
#include "../CommonApple/CommonApple.h"

#include "TProfiler.h"
#include "platform.h"

#include "keycode.h"


struct KeyEvent
{
    char c;
    KeyCode  code;
    bool     KeyShift;
    bool     KeyCtrl;
    bool     KeyAlt;
    bool     KeyCommand;
};


static KeyEvent ConvertKeyEvent(NSEvent *event)
{
    NSString* str = event.characters;
    
    KeyEvent ke;
    ke.c = [str characterAtIndex:0];
    ke.code = ConvertKeyCode_OSX(event.keyCode);
    ke.KeyCtrl = (event.modifierFlags & NSEventModifierFlagControl) != 0;
    ke.KeyShift = (event.modifierFlags & NSEventModifierFlagShift) != 0;
    ke.KeyAlt = (event.modifierFlags & NSEventModifierFlagOption) != 0;
    ke.KeyCommand = (event.modifierFlags & NSEventModifierFlagCommand) != 0;
    return ke;
}

@implementation RenderViewControllerOSX
{
    render::ContextPtr _context;
    render::ContextPtr _gl_context;
    render::metal::IMetalContextPtr _metal_context;
    NSTrackingArea *_trackingArea;
    IVizControllerPtr _vizController;
}


- (void)keyDown:(NSEvent *)event
{
    PROFILE_FUNCTION_CAT("input")

    KeyEvent ke = ConvertKeyEvent(event);

    ImGuiIO& io = ImGui::GetIO();
    io.KeyCtrl      = ke.KeyCtrl;
    io.KeyShift     = ke.KeyShift;
    io.KeyAlt       = ke.KeyAlt;
    io.KeySuper     = ke.KeyCommand;
    if (ke.code != 0)
    {
       io.KeysDown[ke.code] = true;
    }


    NSString* str = event.characters;
    int len = (int)[str length];
    for (int i = 0; i < len; i++)
    {
        int c = [str characterAtIndex:i];
        if (!io.KeyCtrl && !(c >= 0xF700 && c <= 0xFFFF))
            io.AddInputCharacter((unsigned int)c);
    }
}

- (void)keyUp:(NSEvent *)event
{
    PROFILE_FUNCTION_CAT("input")

    KeyEvent ke = ConvertKeyEvent(event);
    
    ImGuiIO& io = ImGui::GetIO();
    io.KeyCtrl      = ke.KeyCtrl;
    io.KeyShift     = ke.KeyShift;
    io.KeyAlt       = ke.KeyAlt;
    io.KeySuper     = ke.KeyCommand;
    if (ke.code != 0)
    {
       io.KeysDown[ke.code] = false;
    }
}


-(void)onDragDrop:(NSArray * _Nonnull)files
{
//    [self.visualizer onDragDrop:files];
}


- (void)onMouseEvent:(NSEvent * _Nonnull)event
{
    PROFILE_FUNCTION_CAT("input")
    ImGui_ImplOSX_HandleEvent(event, self.view);
}


- (void)mouseMoved:(NSEvent *)event {
    [self onMouseEvent:event];
}

- (void)mouseDown:(NSEvent *)event {
    [self onMouseEvent:event];
}

- (void)mouseUp:(NSEvent *)event {
    [self onMouseEvent:event];
}

- (void)mouseDragged:(NSEvent *)event {
    [self onMouseEvent:event];
}

- (void)scrollWheel:(NSEvent *)event {
    [self onMouseEvent:event];
}


bool UseGL = false;

- (void)viewDidLoad
{
    PROFILE_FUNCTION_CAT("load")

    
    [super viewDidLoad];
    
    
    Config::TryGetBool("OPENGL", &UseGL);

    
    if (!UseGL)
    {
        [self initMetal];
    }
    else
    {
        [self initGL];
    }
    
    
    
    std::string resourceDir;
    GetResourceDir(resourceDir);
    std::string assetDir =  PathCombine(resourceDir,  "assets");
    std::string userDir;
    GetApplicationSupportDir(userDir);
    _vizController = CreateVizController(_context, assetDir, userDir);
    
    
    ImGui_ImplOSX_Init();
    
    
    
    
    
    // Add a tracking area in order to receive mouse events whenever the mouse is within the bounds of our view
    _trackingArea = [[NSTrackingArea alloc] initWithRect:NSZeroRect
                                                 options:NSTrackingMouseMoved | NSTrackingInVisibleRect | NSTrackingActiveAlways
                                                   owner:self
                                                userInfo:nil];
    [self.view addTrackingArea:_trackingArea];

    [self.view becomeFirstResponder];


}

-(void)initGL
{
    PROFILE_FUNCTION_CAT("load")

    NSOpenGLPixelFormatAttribute attrs[] =
    {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 32,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        0
    };

    NSOpenGLPixelFormat * fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: attrs];
    GLView *view = [[GLView alloc] initWithFrame:self.view.frame pixelFormat:fmt];
    
    self.view = view;
    view.delegate = self;
    view.eventDelegate = self;
    [view registerDragDrop];
   
    [view.openGLContext makeCurrentContext];

    
    

    _gl_context = render::gles::GLCreateContext();
//    _context->SetRenderTarget(nullptr);
      _context = _gl_context;
  
}



-(void)initMetal
{
    PROFILE_FUNCTION_CAT("load")


    id <MTLDevice> device = MTLCreateSystemDefaultDevice();
    if(!device)
    {
        NSLog(@"Metal is not supported on this device");
        return;
    }
    MetalView *view = [[MetalView alloc] initWithFrame:self.view.frame device:device];;

    // configure view
//    view.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;

    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
//    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    
//    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;

//    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
//    view.colorPixelFormat = MTLPixelFormatBGR10A2Unorm;
//    view.colorPixelFormat =MTLPixelFormatRGBA32Float;

    
    view.depthStencilPixelFormat = MTLPixelFormatInvalid;
    view.sampleCount = 1;
    view.framebufferOnly = YES;
    view.delegate = self;
    view.preferredFramesPerSecond = 60.0f;
    
    view.presentsWithTransaction = NO;
    
    view.eventDelegate = self;
    [view registerDragDrop];

    
    self.view = view;

#if 1
//    if (Config::GetBool("HDR", true))
    {
        CAMetalLayer *layer = (CAMetalLayer *)view.layer;

        layer.wantsExtendedDynamicRangeContent= YES;
    //    layer.pixelFormat = view.colorPixelFormat;
        const CFStringRef name = kCGColorSpaceExtendedSRGB;
    //    const CFStringRef name = kCGColorSpaceExtendedLinearSRGB;
    //    const CFStringRef name = kCGColorSpaceSRGB;
    //    const CFStringRef name = kCGColorSpaceSRGB;

        CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(name);
        layer.colorspace = colorspace;
        CGColorSpaceRelease(colorspace);
        
        view.colorPixelFormat = MTLPixelFormatRGBA16Float;
    }
#endif

    
    _metal_context = render::metal::MetalCreateContext(device);
    _context = _metal_context;
    

    _metal_context->SetView( view );
//    _context->SetRenderTarget(nullptr);

}



- (void)drawInGLView:(GLView * _Nonnull)view
{
    @autoreleasepool {
        PROFILE_FRAME()

        ImGui_ImplOSX_NewFrame(view);


      //    [view bindDrawable];

          NSRect backingBounds = [view convertRectToBacking:[view bounds]];
          GLsizei width  = (GLsizei)(backingBounds.size.width);
          GLsizei height = (GLsizei)(backingBounds.size.height);
          float scale = view.window.backingScaleFactor;
        _context->SetDisplayInfo(
            {
            .size = Size2D(width, height),
            .format = render::PixelFormat::RGBA8Unorm,
            .refreshRate = 60.0f,
            .scale = scale,
            .samples = 1,
            .maxEDR = 1.0f
            }
            );

//          _context->SetView( _view );
        _context->BeginScene();
        _vizController->Render(0, 1);
        _context->EndScene();
        _context->Present();


//        if (_vizController->ShouldQuit())
//        {
//            NSWindow *window = view.window;
//            [window close];
//        }
  }
}


- (void)drawInMTKView:(nonnull MTKView *)view
{
    @autoreleasepool {
        PROFILE_FRAME()

        ImGui_ImplOSX_NewFrame(view);
        


//            NSScreen *screen = view.window.screen;
//            printf("maximumExtendedDynamicRangeColorComponentValue %f\n",
//                   screen.maximumExtendedDynamicRangeColorComponentValue
//        //           screen.maximumPotentialExtendedDynamicRangeColorComponentValue,
//        //           screen.maximumReferenceExtendedDynamicRangeColorComponentValue
//                   );
//
     
        _metal_context->SetView( view );
        _context->BeginScene();
        _vizController->Render(0, 1);
        _context->EndScene();
        _context->Present();


//        if (_vizController->ShouldQuit())
//        {
//            NSWindow *window = view.window;
//            [window close];
//        }
    }
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    /// Respond to drawable size or orientation changes here
//    _context->SetDisplaySize(size.width, size.height);
}





@end
