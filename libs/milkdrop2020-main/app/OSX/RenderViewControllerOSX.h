
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import "IVizController.h"
#import "MetalView.h"
#import "GLView.h"

@interface RenderViewControllerOSX : NSViewController <MTKViewDelegate, GLViewDelegate, EventDelegate>

@end
