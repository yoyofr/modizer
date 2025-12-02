//
//  ModizerMacWindowManager.m
//  modizer
//
//  Created by Yohann Magnien David on 29/11/2025.
//

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#import "../ModizerConstants.h"

@interface ModizerMacWindowManager : NSObject
+ (void)setAlwaysOnTop:(BOOL)enabled;
+ (void)enableAlwaysOnTop;
+ (void)disableAlwaysOnTop;
@end

@implementation ModizerMacWindowManager

+ (void)setAlwaysOnTop:(BOOL)enabled {
#ifdef MDZ_MACOS_WINDOW_AOT
    NSArray *windows = [NSApp windows];
    
    for (NSWindow *window in windows) {
        if (enabled) {
            // NSFloatingWindowLevel = 3 (au-dessus des fenêtres normales)
            // NSStatusWindowLevel = 25 (encore plus haut, comme les menubar items)
            [window setLevel:NSFloatingWindowLevel];
            
            // Optionnel: rendre visible sur tous les espaces
            [window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
                                          NSWindowCollectionBehaviorFullScreenAuxiliary];
        } else {
            [window setLevel:NSNormalWindowLevel];
            [window setCollectionBehavior:NSWindowCollectionBehaviorDefault];
        }
    }
#endif
}

+ (void)enableAlwaysOnTop {
    [self setAlwaysOnTop:YES];
}

+ (void)disableAlwaysOnTop {
    [self setAlwaysOnTop:NO];
}

// Auto-load quand le bundle est chargé
+ (void)load {
    //NSLog(@"ModizerMacWindowPlugin loaded");
}

@end
