

#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

#include <stdio.h>

#include "render/context.h"
#include "render/context_metal.h"

#include "VizController.h"
#include "audio/IAudioSource.h"


int main(int argc, const char * argv[]) 
{
	// TODO
	printf("M1lkdr0p\n");

    id <MTLDevice> device = MTLCreateSystemDefaultDevice();
    if(!device)
    {
        NSLog(@"Metal is not supported on this device");
        return -1;
    }
    
    render::metal::IMetalContextPtr context = render::metal::MetalCreateContext(device);

    // fake it...
    context->SetDisplayInfo(
        {
        .size = Size2D(2048, 2048),
        .format = render::PixelFormat::RGBA8Unorm,
        .refreshRate = 60.0f,
        .scale = 1,
        .samples = 1,
        .maxEDR = 1.0f
        }
    );

    
    
    std::string resourceDir = "..";
    std::string pluginDir =  resourceDir + "/assets";
    std::string userDir= "user";

    
//    std::string firstPreset = "Standard/Aderrasi - Visitor";
    
    
    auto vizController = CreateVizController(context, pluginDir, userDir);

    vizController->TestAllPresets(
                                  [](std::string name, std::string error) {
        
                                    }
                                  
                                  );
    
//    printf("1\n");
//    vizController->SelectPreset(firstPreset);
//    printf("2\n");
//    vizController->SetSelectionLock(true);
//    printf("3\n");
    
    return 0;
}
