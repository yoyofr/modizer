

#import <Foundation/Foundation.h>



#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>
#include "render/MDcontext.h"
#include "render/context_metal.h"

#import "ExternalViewController.h"


@implementation ExternalViewController
{
    id <MTLDevice> _device;
    MTKView *_metal_view;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [self initMetal];
}

-(void)initMetal
{

      id <MTLDevice> device = MTLCreateSystemDefaultDevice();
      if(!device)
      {
          NSLog(@"Metal is not supported on this device");
          return;
      }
      MTKView *view = [[MTKView alloc] initWithFrame:self.view.frame device:device];;
      
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
      

      self.view = view;
      
      
      
      

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

      
      
      
      
      _metal_view = view;
      
    
//    _view = (MTKView *)self.view;
//    _device = MTLCreateSystemDefaultDevice();
//    _view.device = _device;
//
//    _view.depthStencilPixelFormat = MTLPixelFormatInvalid;
//    _view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
//    _view.sampleCount = 1;
//
////    _view.contentMode = UIViewContentModeScaleToFill;
//
//    auto view = _view;
//    NSLog(@"viewDidLoad: %@ %fx%f %fx%f\n", view,
//          view.bounds.size.width, view.bounds.size.height,
//             view.window.bounds.size.width,
//             view.window.bounds.size.height
//             );
//
//    _view.delegate = self;
//
}
    
-(void)draw
{
    [self drawInMTKView:_metal_view];
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    if (self.visualizer && self.context)
    {
       
//        if (!view.autoResizeDrawable)
//        {
//            UIWindow *window = view.window;
//            UIScreen * screen = window.screen;
//            if (!screen) screen = UIScreen.mainScreen;
//            float scale = screen.scale;
//            CGSize size = view.bounds.size;
//            size.width  *= scale;
//            size.height *= scale;
//            view.drawableSize = size;
//        }
        
        self.context->SetView(view);
        self.context->BeginScene();
        self.visualizer->Render(1, 2);
        self.context->EndScene();
        self.context->Present();
    }
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{

    NSLog(@"drawableSizeWillChange: %@ %fx%f %fx%f\n", view, size.width, size.height,
          view.window.bounds.size.width,
          view.window.bounds.size.height
          );
}


@end

