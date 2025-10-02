

#pragma once

#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>

#import "RenderViewControllerIOS.h"
#include "render/context_metal.h"
#include "IVizController.h"

@interface ExternalViewController : UIViewController <MTKViewDelegate> {
    
}


@property (nonatomic)  IVizControllerPtr  visualizer;
@property (nonatomic) render::metal::IMetalContextPtr context;

-(void)draw;


@end

