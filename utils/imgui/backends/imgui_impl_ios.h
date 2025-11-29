// dear imgui: Platform Backend for OSX / Cocoa
// This needs to be used along with a Renderer (e.g. OpenGL2, OpenGL3, Vulkan, Metal..)
// [ALPHA] Early backend, not well tested. If you want a portable application, prefer using the GLFW or SDL platform Backends on Mac.

// Implemented features:
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [X] Platform: OSX clipboard is supported within core Dear ImGui (no specific code in this backend).
// Issues:
//  [ ] Platform: Keys are all generally very broken. Best using [event keycode] and not [event characters]..
//  [ ] Platform: Multi-viewport / platform windows.

// You can copy and use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#ifndef __imgui_impl_ios_h__
#define __imgui_impl_ios_h__

#include "imgui.h"      // IMGUI_IMPL_API

#define FONT_TRACKER_NB 5

#ifndef ImGuiIOSEvent_t
#define ImGuiIOSEvent_t
struct ImGuiIOSEvent {
    int event_type;
    int tap_nb;
    float pos_x,pos_y;
    int delta_x,delta_y;
    float wheel_x,wheel_y;
};
#endif

#ifndef ImGuiIOSEventType_t
#define ImGuiIOSEventType_t
enum ImGuiIOSEventType {
    IMGUI_IOS_Event_None,
    IMGUI_IOS_Event_Tap_1,
    IMGUI_IOS_Event_MouseMove,
    IMGUI_IOS_Event_MouseDrag,
    IMGUI_IOS_Event_MouseWheel,
    IMGUI_IOS_Event_Swipe
};
#endif


IMGUI_IMPL_API bool ImGui_ImplIOS_Init();
IMGUI_IMPL_API void ImGui_ImplIOS_Shutdown();
IMGUI_IMPL_API void ImGui_ImplIOS_NewFrame(float w,float h,float scale);
IMGUI_IMPL_API void ImGui_ImplIOS_UpdateEvent(ImGuiIOSEvent *event);
IMGUI_IMPL_API void ImGui_ImplIOS_ResetKeyMouse();

void ImGui_ImplIOS_UpdateEvent(ImGuiIOSEvent *event);
void ImGui_ImplIOS_ResetTapPos();

#ifdef __OBJC__

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface ImGui_ImplIOS_UI : UIViewController <UITextFieldDelegate> {
    
}

@property (nonatomic, strong) UIView *mainView;
@property (nonatomic, strong) UITextField *textField;
@property (nonatomic, strong) NSString *text;
@property (nonatomic, assign) BOOL isActive;
@property (nonatomic, assign) int cursorPos;

- (void)updateEvent;
- (void)initTF:(UIView *)view;

@end

#endif
#endif

