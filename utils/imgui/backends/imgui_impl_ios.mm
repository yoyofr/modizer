#include "imgui.h"
#include "imgui_impl_ios.h"

#include "StopWatch.h"

#include "MDZFontAwesome.h"

#include "ModizerConstants.H"

#import <UIKit/UIKit.h>

#define FONT_MENU_FILE  @"Fonts/Roboto-Medium"
#define FONT_MENU_FILE_JAP @"Fonts/Mplus1-Medium"

#define MAX_LASTCHAR_SIZE 16 //buffer to capture key inputs in UITextfield

extern float glScaleFactor;
static StopWatch g_timer;
static ImGuiIOSEvent currentEvent;
static int wantInputText=0;
static unichar lastChar[MAX_LASTCHAR_SIZE];
static volatile int lastChar_pos;
int mouseMoveInProgress;

NSMutableArray *mac_key_pressed,*mac_key_released;


float mdz_font_size[4]={10,15,22,30};
//ImFont *font_icon[4];
ImFont  *font_menu;
ImFont  *font_menu_icon;
ImFont  *font_tracker[FONT_TRACKER_NB];
ImFont  *font_trackerH[FONT_TRACKER_NB];

const char *font_trackerName[FONT_TRACKER_NB][2]={
    {"ModernDOS8x16","DOS"},
    {"amiga4ever","Amiga"},
    {"Commodore Rounded v1.2","C64"},
    {"Coda-Regular","Coda"},
    {"Roboto-Medium","Roboto"}
};

float font_trackerSize[FONT_TRACKER_NB][5]={
    {16.0,  9.0,    1.0,    0.0,    0.0},
    {16.0,  14.0,   1.0,    0.0,    0.0},
    {16.0,  12.0,   1.0,    0.0,    -2.0},
    {16.0,  8.0,    1.0,    0.0,    -1.0},
    {16.0,  10.0,   1.0,    0.0,    -1.0},
};

// Functions
const char* ImGui_ImplIOS_GetClipboardText(void* user_data);
void ImGui_ImplIOS_SetClipboardText(void* user_data, const char* text);

bool ImGui_ImplIOS_Init()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup backend capabilities flags
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    //io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
    //io.BackendFlags |= ImGuiConfigFlags_IsTouchScreen;
    io.BackendFlags |= ImGuiConfigFlags_IsTouchScreen;
    io.BackendPlatformName = "imgui_impl_ios";
    g_timer.Restart();
    
    mac_key_pressed=[NSMutableArray array];
    mac_key_released=[NSMutableArray array];
    
    io.SetClipboardTextFn = ImGui_ImplIOS_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplIOS_GetClipboardText;
    io.ClipboardUserData = nullptr;
    
    currentEvent.event_type=IMGUI_IOS_Event_None;
    
    font_menu=io.Fonts->AddFontFromFileTTF([[[NSBundle mainBundle] pathForResource:FONT_MENU_FILE_JAP ofType: @"ttf"] UTF8String], 16.0f*glScaleFactor, NULL, io.Fonts->GetGlyphRangesJapanese());
    IM_ASSERT(font_menu != NULL);
    
    ImFontConfig icons_config = ImFontConfig();
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.OversampleH = 1;
    icons_config.OversampleV = 1;
    icons_config.GlyphOffset.y = -4;
    icons_config.GlyphExtraAdvanceX = 1;
    static const ImWchar ranges[] =
      {
          ICON_MIN_FA, ICON_MAX_FA,
          0,
      };
    font_menu_icon=io.Fonts->AddFontFromFileTTF([[[NSBundle mainBundle] pathForResource:@"Fonts/fontawesome-webfont" ofType: @"ttf"] UTF8String], 10.0f*glScaleFactor, &icons_config, ranges);
    IM_ASSERT(font_menu_icon != NULL);
    
    for (int i=0;i<FONT_TRACKER_NB;i++) {
        //NSLog(@"loading %s.ttf size: %f",font_trackerName[i][0],ft_size);
        ImFontConfig font_cfg = ImFontConfig();
        font_cfg.GlyphMinAdvanceX=font_trackerSize[i][1]*glScaleFactor*font_trackerSize[i][2];
        font_cfg.GlyphMaxAdvanceX=font_trackerSize[i][1]*glScaleFactor*font_trackerSize[i][2];
        font_cfg.PixelSnapH = true;
        font_cfg.OversampleH = 1;
        font_cfg.OversampleV = 1;
        
        font_tracker[i] = io.Fonts->AddFontFromFileTTF([[[NSBundle mainBundle] pathForResource:[NSString stringWithFormat:@"Fonts/%s",font_trackerName[i][0]] ofType: @"ttf"] UTF8String],
                                                       16.0f*glScaleFactor,
                                                       &font_cfg, io.Fonts->GetGlyphRangesDefault());
        IM_ASSERT(font_tracker[i] != NULL);
        
        font_cfg = ImFontConfig();
        font_cfg.GlyphMinAdvanceX=font_trackerSize[i][1]*glScaleFactor*font_trackerSize[i][2];
        font_cfg.GlyphMaxAdvanceX=font_trackerSize[i][1]*glScaleFactor*font_trackerSize[i][2];
        font_cfg.PixelSnapH = true;
        font_cfg.OversampleH = 1;
        font_cfg.OversampleV = 1;
        
        font_trackerH[i] = io.Fonts->AddFontFromFileTTF([[[NSBundle mainBundle] pathForResource:[NSString stringWithFormat:@"Fonts/%s",font_trackerName[i][0]] ofType: @"ttf"] UTF8String],
                                                        (16.0f*glScaleFactor*1.5f),
                                                        &font_cfg, io.Fonts->GetGlyphRangesDefault());
        IM_ASSERT(font_trackerH[i] != NULL);
    }

    return true;
}

void ImGui_ImplIOS_Shutdown()
{
}

ImGuiKey ImGui_ImplIOS_KeyCodeToImGuiKey(int key_code)
{
    switch (key_code)
    {
        case UIKeyboardHIDUsageKeyboardA: return ImGuiKey_A;
        case UIKeyboardHIDUsageKeyboardS: return ImGuiKey_S;
        case UIKeyboardHIDUsageKeyboardD: return ImGuiKey_D;
        case UIKeyboardHIDUsageKeyboardF: return ImGuiKey_F;
        case UIKeyboardHIDUsageKeyboardH: return ImGuiKey_H;
        case UIKeyboardHIDUsageKeyboardG: return ImGuiKey_G;
        case UIKeyboardHIDUsageKeyboardZ: return ImGuiKey_Z;
        case UIKeyboardHIDUsageKeyboardX: return ImGuiKey_X;
        case UIKeyboardHIDUsageKeyboardC: return ImGuiKey_C;
        case UIKeyboardHIDUsageKeyboardV: return ImGuiKey_V;
        case UIKeyboardHIDUsageKeyboardB: return ImGuiKey_B;
        case UIKeyboardHIDUsageKeyboardQ: return ImGuiKey_Q;
        case UIKeyboardHIDUsageKeyboardW: return ImGuiKey_W;
        case UIKeyboardHIDUsageKeyboardE: return ImGuiKey_E;
        case UIKeyboardHIDUsageKeyboardR: return ImGuiKey_R;
        case UIKeyboardHIDUsageKeyboardY: return ImGuiKey_Y;
        case UIKeyboardHIDUsageKeyboardT: return ImGuiKey_T;
        case UIKeyboardHIDUsageKeyboard1: return ImGuiKey_1;
        case UIKeyboardHIDUsageKeyboard2: return ImGuiKey_2;
        case UIKeyboardHIDUsageKeyboard3: return ImGuiKey_3;
        case UIKeyboardHIDUsageKeyboard4: return ImGuiKey_4;
        case UIKeyboardHIDUsageKeyboard6: return ImGuiKey_6;
        case UIKeyboardHIDUsageKeyboard5: return ImGuiKey_5;
        case UIKeyboardHIDUsageKeyboardEqualSign: return ImGuiKey_Equal;
        case UIKeyboardHIDUsageKeyboard9: return ImGuiKey_9;
        case UIKeyboardHIDUsageKeyboard7: return ImGuiKey_7;
        case UIKeyboardHIDUsageKeyboardHyphen: return ImGuiKey_Minus;
        case UIKeyboardHIDUsageKeyboard8: return ImGuiKey_8;
        case UIKeyboardHIDUsageKeyboard0: return ImGuiKey_0;
        case UIKeyboardHIDUsageKeyboardCloseBracket: return ImGuiKey_RightBracket;
        case UIKeyboardHIDUsageKeyboardO: return ImGuiKey_O;
        case UIKeyboardHIDUsageKeyboardU: return ImGuiKey_U;
        case UIKeyboardHIDUsageKeyboardOpenBracket: return ImGuiKey_LeftBracket;
        case UIKeyboardHIDUsageKeyboardI: return ImGuiKey_I;
        case UIKeyboardHIDUsageKeyboardP: return ImGuiKey_P;
        case UIKeyboardHIDUsageKeyboardL: return ImGuiKey_L;
        case UIKeyboardHIDUsageKeyboardJ: return ImGuiKey_J;
        case UIKeyboardHIDUsageKeyboardQuote: return ImGuiKey_Apostrophe;
        case UIKeyboardHIDUsageKeyboardK: return ImGuiKey_K;
        case UIKeyboardHIDUsageKeyboardSemicolon: return ImGuiKey_Semicolon;
        case UIKeyboardHIDUsageKeyboardBackslash: return ImGuiKey_Backslash;
        case UIKeyboardHIDUsageKeyboardComma: return ImGuiKey_Comma;
        case UIKeyboardHIDUsageKeyboardSlash: return ImGuiKey_Slash;
        case UIKeyboardHIDUsageKeyboardN: return ImGuiKey_N;
        case UIKeyboardHIDUsageKeyboardM: return ImGuiKey_M;
        case UIKeyboardHIDUsageKeyboardPeriod: return ImGuiKey_Period;
        case UIKeyboardHIDUsageKeyboardGraveAccentAndTilde: return ImGuiKey_GraveAccent;
        case UIKeyboardHIDUsageKeypadPeriod: return ImGuiKey_KeypadDecimal;
        case UIKeyboardHIDUsageKeypadAsterisk: return ImGuiKey_KeypadMultiply;
        case UIKeyboardHIDUsageKeypadPlus: return ImGuiKey_KeypadAdd;
        case UIKeyboardHIDUsageKeypadNumLock: return ImGuiKey_NumLock;
        case UIKeyboardHIDUsageKeypadSlash: return ImGuiKey_KeypadDivide;
        case UIKeyboardHIDUsageKeypadEnter: return ImGuiKey_KeypadEnter;
        case UIKeyboardHIDUsageKeypadHyphen: return ImGuiKey_KeypadSubtract;
        case UIKeyboardHIDUsageKeypadEqualSign: return ImGuiKey_KeypadEqual;
        case UIKeyboardHIDUsageKeypad0: return ImGuiKey_Keypad0;
        case UIKeyboardHIDUsageKeypad1: return ImGuiKey_Keypad1;
        case UIKeyboardHIDUsageKeypad2: return ImGuiKey_Keypad2;
        case UIKeyboardHIDUsageKeypad3: return ImGuiKey_Keypad3;
        case UIKeyboardHIDUsageKeypad4: return ImGuiKey_Keypad4;
        case UIKeyboardHIDUsageKeypad5: return ImGuiKey_Keypad5;
        case UIKeyboardHIDUsageKeypad6: return ImGuiKey_Keypad6;
        case UIKeyboardHIDUsageKeypad7: return ImGuiKey_Keypad7;
        case UIKeyboardHIDUsageKeypad8: return ImGuiKey_Keypad8;
        case UIKeyboardHIDUsageKeypad9: return ImGuiKey_Keypad9;
        case UIKeyboardHIDUsageKeyboardReturnOrEnter: return ImGuiKey_Enter;
        case UIKeyboardHIDUsageKeyboardTab: return ImGuiKey_Tab;
        case UIKeyboardHIDUsageKeyboardSpacebar: return ImGuiKey_Space;
        case UIKeyboardHIDUsageKeyboardDeleteOrBackspace: return ImGuiKey_Backspace;
        case UIKeyboardHIDUsageKeyboardEscape: return ImGuiKey_Escape;
        case UIKeyboardHIDUsageKeyboardCapsLock: return ImGuiKey_CapsLock;
        case UIKeyboardHIDUsageKeyboardLeftControl: return ImGuiKey_LeftCtrl;
        case UIKeyboardHIDUsageKeyboardLeftShift: return ImGuiKey_LeftShift;
        case UIKeyboardHIDUsageKeyboardLeftAlt: return ImGuiKey_LeftAlt;
        case UIKeyboardHIDUsageKeyboardLeftGUI: return ImGuiKey_LeftSuper;
        case UIKeyboardHIDUsageKeyboardRightControl: return ImGuiKey_RightCtrl;
        case UIKeyboardHIDUsageKeyboardRightShift: return ImGuiKey_RightShift;
        case UIKeyboardHIDUsageKeyboardRightAlt: return ImGuiKey_RightAlt;
        case UIKeyboardHIDUsageKeyboardRightGUI: return ImGuiKey_RightSuper;
//      case UIKeyboardHIDUsageKeyboardFunction: return ImGuiKey_;
//      case UIKeyboardHIDUsageKeyboardVolumeUp: return ImGuiKey_;
//      case UIKeyboardHIDUsageKeyboardVolumeDown: return ImGuiKey_;
//      case UIKeyboardHIDUsageKeyboardMute: return ImGuiKey_;
        case UIKeyboardHIDUsageKeyboardF1: return ImGuiKey_F1;
        case UIKeyboardHIDUsageKeyboardF2: return ImGuiKey_F2;
        case UIKeyboardHIDUsageKeyboardF3: return ImGuiKey_F3;
        case UIKeyboardHIDUsageKeyboardF4: return ImGuiKey_F4;
        case UIKeyboardHIDUsageKeyboardF5: return ImGuiKey_F5;
        case UIKeyboardHIDUsageKeyboardF6: return ImGuiKey_F6;
        case UIKeyboardHIDUsageKeyboardF7: return ImGuiKey_F7;
        case UIKeyboardHIDUsageKeyboardF8: return ImGuiKey_F8;
        case UIKeyboardHIDUsageKeyboardF9: return ImGuiKey_F9;
        case UIKeyboardHIDUsageKeyboardF10: return ImGuiKey_F10;
        case UIKeyboardHIDUsageKeyboardF11: return ImGuiKey_F11;
        case UIKeyboardHIDUsageKeyboardF12: return ImGuiKey_F12;
        case UIKeyboardHIDUsageKeyboardF13: return ImGuiKey_F13;
        case UIKeyboardHIDUsageKeyboardF14: return ImGuiKey_F14;
        case UIKeyboardHIDUsageKeyboardF15: return ImGuiKey_F15;
        case UIKeyboardHIDUsageKeyboardF16: return ImGuiKey_F16;
        case UIKeyboardHIDUsageKeyboardF17: return ImGuiKey_F17;
        case UIKeyboardHIDUsageKeyboardF18: return ImGuiKey_F18;
        case UIKeyboardHIDUsageKeyboardF19: return ImGuiKey_F19;
        case UIKeyboardHIDUsageKeyboardF20: return ImGuiKey_F20;
        case UIKeyboardHIDUsageKeyboardMenu: return ImGuiKey_Menu;
        case UIKeyboardHIDUsageKeyboardHelp: return ImGuiKey_Insert;
        case UIKeyboardHIDUsageKeyboardHome: return ImGuiKey_Home;
        case UIKeyboardHIDUsageKeyboardPageUp: return ImGuiKey_PageUp;
        case UIKeyboardHIDUsageKeyboardDeleteForward: return ImGuiKey_Delete;
        case UIKeyboardHIDUsageKeyboardEnd: return ImGuiKey_End;
        case UIKeyboardHIDUsageKeyboardPageDown: return ImGuiKey_PageDown;
        case UIKeyboardHIDUsageKeyboardLeftArrow: return ImGuiKey_LeftArrow;
        case UIKeyboardHIDUsageKeyboardRightArrow: return ImGuiKey_RightArrow;
        case UIKeyboardHIDUsageKeyboardDownArrow: return ImGuiKey_DownArrow;
        case UIKeyboardHIDUsageKeyboardUpArrow: return ImGuiKey_UpArrow;
        default: return ImGuiKey_None;
    }
}

ImGuiKey ImGui_ImplIOS_KeyCodeToKeyModCode(ImGuiKey imkey) {
    ImGuiKey imkeyMod=ImGuiKey_None;
    switch (imkey) {
        case ImGuiKey_LeftShift:
        case ImGuiKey_RightShift:
            imkeyMod=ImGuiMod_Shift;
            break;
        case ImGuiKey_LeftCtrl:
        case ImGuiKey_RightCtrl:
            imkeyMod=ImGuiMod_Ctrl;
            break;
        case ImGuiKey_LeftAlt:
        case ImGuiKey_RightAlt:
            imkeyMod=ImGuiMod_Alt;
            break;
        case ImGuiKey_LeftSuper:
        case ImGuiKey_RightSuper:
            imkeyMod=ImGuiMod_Super;
            break;
        default:
            break;
    }
    return imkeyMod;
}

void ImGui_ImplIOS_ResetKeyMouse() {
    ImGuiIO& io = ImGui::GetIO();
        
        // Clavier
        memset(io.KeysData, 0, sizeof(io.KeysData));
        io.KeyCtrl = false;
        io.KeyShift = false;
        io.KeyAlt = false;
        io.KeySuper = false;
        
        // Souris
        for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) {
            io.MouseDown[i] = false;
        }
        
        // Optionnel : réinitialiser aussi les down duration
        for (int i = 0; i < IM_ARRAYSIZE(io.KeysData); i++) {
            io.KeysData[i].Down = false;
            io.KeysData[i].DownDuration = -1.0f;
            io.KeysData[i].DownDurationPrev = -1.0f;
        }
}

void ImGui_ImplIOS_UpdateEvent(ImGuiIOSEvent *event)
{
    static float lastMouseX=-1,lastMouseY=-1;
    static int mouseEventOnHold=0;
    static int mouseShouldReleaseLeftClick=0;
    if (event) {
        currentEvent=*event;
    }
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    //io.WantCaptureMouse=true;
    
    mouseMoveInProgress=false;
    if (currentEvent.event_type==IMGUI_IOS_Event_Tap_1) {
        //        MDZILog("tap1 at %f %f",currentEvent.pos_x,currentEvent.pos_y);
        //        MDZILog("left click pressed");
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);//TouchScreen);
        io.AddMousePosEvent(currentEvent.pos_x,currentEvent.pos_y);
        io.AddMouseButtonEvent(0, true);
        mouseShouldReleaseLeftClick=1;
        mouseEventOnHold=0;
    } else if (currentEvent.event_type==IMGUI_IOS_Event_MouseMove) {
        //        MDZILog("move at %f %f",currentEvent.pos_x,currentEvent.pos_y);
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);//TouchScreen);
        io.AddMousePosEvent(currentEvent.pos_x,currentEvent.pos_y);
        lastMouseX=currentEvent.pos_x;
        lastMouseY=currentEvent.pos_y;
        mouseEventOnHold=0;
        mouseMoveInProgress=true;
    }  else if (currentEvent.event_type==IMGUI_IOS_Event_MouseDrag) {
        //        MDZILog("drag at %f %f",currentEvent.pos_x,currentEvent.pos_y);
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);//TouchScreen);
        io.AddMousePosEvent(currentEvent.pos_x,currentEvent.pos_y);
        io.AddMouseButtonEvent(0, true);
        mouseShouldReleaseLeftClick=1;
        mouseEventOnHold=0;
        mouseMoveInProgress=true;
    } else if (currentEvent.event_type==IMGUI_IOS_Event_MouseWheel) {
        //        MDZILog("wheel %f %f at %f %f",currentEvent.wheel_x,currentEvent.wheel_y,currentEvent.pos_x,currentEvent.pos_y);
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);//TouchScreen);
        io.AddMouseWheelEvent(currentEvent.wheel_x, currentEvent.wheel_y);
        io.AddMousePosEvent(currentEvent.pos_x,currentEvent.pos_y);
        mouseEventOnHold=0;
        //mouseMoveInProgress=true;
    } else if (!mouseEventOnHold) {
        //        MDZILog("Mouse event end");
        if (mouseShouldReleaseLeftClick) {
            //            MDZILog("left click release");
            io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);//TouchScreen);
            io.AddMouseButtonEvent(0, false);
            io.AddMousePosEvent(lastMouseX, lastMouseY);
            mouseShouldReleaseLeftClick=0;
        }
        mouseEventOnHold=1;
    }
    
    
        while ([mac_key_pressed count]) {
            NSNumber *nb=[mac_key_pressed objectAtIndex:0];
            int key=[nb intValue];
            ImGuiKey imkey=ImGui_ImplIOS_KeyCodeToImGuiKey(key);
//            MDZILog("press: %d",imkey);
            io.AddKeyEvent(imkey,true);
            
            ImGuiKey imkeyMod=ImGui_ImplIOS_KeyCodeToKeyModCode(imkey);
            if (imkeyMod!=ImGuiKey_None) io.AddKeyEvent(imkeyMod,true);
            
            [mac_key_pressed removeObjectAtIndex:0];
        }
        while ([mac_key_released count]) {
            NSNumber *nb=[mac_key_released objectAtIndex:0];
            int key=[nb intValue];
            ImGuiKey imkey=ImGui_ImplIOS_KeyCodeToImGuiKey(key);
//            MDZILog("release: %d",imkey);
            io.AddKeyEvent(imkey,false);
            
            ImGuiKey imkeyMod=ImGui_ImplIOS_KeyCodeToKeyModCode(imkey);
            if (imkeyMod!=ImGuiKey_None) io.AddKeyEvent(imkeyMod,false);
            
            [mac_key_released removeObjectAtIndex:0];
        }
    
    if (io.WantTextInput) {
        if (wantInputText==0) wantInputText=1;
        if (wantInputText==2) {
            int pos=0;
            while (lastChar_pos) {
                //NSLog(@"key: %C",lastChar[pos]);
                if (lastChar[pos]>=32) io.AddInputCharacter(lastChar[pos]);
                else {
                    if (lastChar[pos]==0x8) { //Backspace
                        io.AddKeyEvent(ImGuiKey_Backspace,true);
                        io.AddKeyEvent(ImGuiKey_Backspace,false);
                    } else if (lastChar[pos]==0xD) { //Return
                        io.AddKeyEvent(ImGuiKey_Enter,true);
                        io.AddKeyEvent(ImGuiKey_Enter,false);
                    }
                }
                pos++;
                lastChar_pos--;
            }
#if 0
#endif
        }
    } else wantInputText=0;
}

void ImGui_ImplIOS_ResetTapPos() {
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    io.WantCaptureMouse=true;
    
    currentEvent.event_type=IMGUI_IOS_Event_None;
    io.MousePos = ImVec2((float)0, (float)0);
    io.MouseDown[0] = 0;
}


void ImGui_ImplIOS_NewFrame(float w,float h,float scale)
{
    ImGuiIO &io = ImGui::GetIO();

    io.DisplaySize.x = w / scale;
    io.DisplaySize.y = h / scale ;
    io.DisplayFramebufferScale = ImVec2(scale, scale);
    
    io.DeltaTime = g_timer.GetElapsedSeconds();
    g_timer.Restart();
}

@implementation ImGui_ImplIOS_UI

- (void)textFieldDidEndEditing:(UITextField *)textField {
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField{
    lastChar[lastChar_pos]=0xD;
    if (lastChar_pos<(MAX_LASTCHAR_SIZE-1)) lastChar_pos++;
    [textField resignFirstResponder];
    return YES;
}

- (void)textFieldTextChanged:(UITextField *)textField {
    NSString *txt=textField.text;
    int diff=(int)([txt length]-[self.text length]);
    if (diff>0) {
        while (diff) {
            lastChar[lastChar_pos]=[txt characterAtIndex:[txt length]-diff];
            diff--;
            if (lastChar_pos<(MAX_LASTCHAR_SIZE-1)) lastChar_pos++;
        }
    } else while (diff<0) {
        lastChar[lastChar_pos]=8; //Backspace
        diff++;
        if (lastChar_pos<(MAX_LASTCHAR_SIZE-1)) lastChar_pos++;
    }
    _textField.text=@"12";
    self.text=@"12";
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    static int no_rentrant=0;
    if (no_rentrant) return;
    if([keyPath isEqualToString:@"selectedTextRange"] && _textField == object) {
        no_rentrant=1;
        UITextPosition *newPosition = [_textField positionFromPosition:_textField.endOfDocument offset:0];
        [_textField setSelectedTextRange:[_textField textRangeFromPosition:newPosition toPosition:newPosition]];
        no_rentrant=0;
    }
}



- (void)viewDidLoad {
    START_PROFILE
    [super viewDidLoad];
    
    _textField=nil;
    END_PROFILE
}

- (void)initTF:(UIView *)view {
    _mainView=view;
    
    _textField = [[UITextField alloc] init];
    _textField.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
    _textField.borderStyle = UITextBorderStyleRoundedRect;
    _textField.font = [UIFont systemFontOfSize:15];
    _textField.autocorrectionType = UITextAutocorrectionTypeNo;
    _textField.keyboardType = UIKeyboardTypeASCIICapable;
    _textField.returnKeyType = UIReturnKeyDone;
    _textField.clearButtonMode = UITextFieldViewModeWhileEditing;
    _textField.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    _textField.delegate = self;
//    _textField.tag=indexPath.section;
    
    [_textField addTarget:self
                 action:@selector(textFieldTextChanged:)
       forControlEvents:UIControlEventEditingChanged];
    
    [_mainView addSubview:_textField];
    
    _textField.delegate=self;
    
    [_textField addObserver:self forKeyPath:@"selectedTextRange" options:NSKeyValueObservingOptionNew|NSKeyValueObservingOptionOld  context:nil];

    _textField.hidden=true;
    
    lastChar_pos=0;
}

- (void)updateEvent {
    if (_textField==nil) return;
    
    if (wantInputText==1) {
        _textField.text=@"12";
        self.text=@"12";
        
        [_textField becomeFirstResponder];
        UITextPosition *newPosition = [_textField positionFromPosition:_textField.endOfDocument offset:0];
        [_textField setSelectedTextRange:[_textField textRangeFromPosition:newPosition toPosition:newPosition]];
        
        wantInputText=2;
    } else if (wantInputText==0) {
        if ([_textField isFirstResponder]) [_textField resignFirstResponder];
    }
}

const char* ImGui_ImplIOS_GetClipboardText(void* user_data)
{
    NSLog(@"get clipboard: ");
    UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
    NSString *text = pasteboard.string;
    NSLog(@"%@",text);
    return [text UTF8String];
}

void ImGui_ImplIOS_SetClipboardText(void* user_data, const char* text)
{
    NSLog(@"set clipboard: %s",text);
    UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
    pasteboard.string=[NSString stringWithUTF8String:text];
}


@end

