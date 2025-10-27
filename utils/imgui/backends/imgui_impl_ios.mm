#include "imgui.h"
#include "imgui_impl_ios.h"

#include "StopWatch.h"

#include "MDZFontAwesome.h"

#include "ModizerConstants.H"

extern float glScaleFactor;
static StopWatch g_timer;
static ImGuiIOSEvent currentEvent;
static int wantInputText=0;
static unichar lastChar;
int mouseMoveInProgress;
int move_cursorL,move_cursorR;
int shiftPressedL,shiftPressedR;
int keyDel;


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

ImGuiKey ImGui_ImplIOS_KeyEventToImGuiKey();

// Functions
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
    
    currentEvent.event_type=IMGUI_IOS_Event_None;
    
        font_menu=io.Fonts->AddFontFromFileTTF([[[NSBundle mainBundle] pathForResource:@"Fonts/Roboto-Medium" ofType: @"ttf"] UTF8String], 16.0f*glScaleFactor, NULL, io.Fonts->GetGlyphRangesDefault());
        IM_ASSERT(font_menu != NULL);
        
        ImFontConfig icons_config = ImFontConfig();
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.OversampleH = 1;
        icons_config.OversampleV = 1;
        static const ImWchar ranges[] =
          {
              ICON_MIN_FA, ICON_MAX_FA,
              0,
          };
        //g_font_awesome = ImGuiSupport_AddFontFromFile(fontPath, fontSize, &icons_config, ranges);
        font_menu_icon=io.Fonts->AddFontFromFileTTF([[[NSBundle mainBundle] pathForResource:@"Fonts/fontawesome-webfont" ofType: @"ttf"] UTF8String], 32.0f*glScaleFactor, &icons_config, ranges);
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


void ImGui_ImplIOS_UpdateEvent(ImGuiIOSEvent *event)
{
    static int mouseEventOnHold=0;
    if (event) {
        currentEvent=*event;
    }
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    //io.WantCaptureMouse=true;
    
    mouseMoveInProgress=false;
    if (currentEvent.event_type==IMGUI_IOS_Event_Tap_1) {
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);//TouchScreen);
        io.AddMousePosEvent((float)(currentEvent.pos_x), (float)(currentEvent.pos_y));
        io.AddMouseButtonEvent(0, true);
        mouseEventOnHold=0;
    } else if (currentEvent.event_type==IMGUI_IOS_Event_MouseMove) {
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);//TouchScreen);
        io.AddMousePosEvent((float)(currentEvent.pos_x), (float)(currentEvent.pos_y));
        io.AddMouseButtonEvent(0, true);
        mouseEventOnHold=0;
        mouseMoveInProgress=true;
    } else if (!mouseEventOnHold) {
        io.AddMouseButtonEvent(0, false);
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);//TouchScreen);
        io.AddMousePosEvent((float)(-1), (float)(-1));
        mouseEventOnHold=1;
    }
    
    if (io.WantTextInput) {
        if (wantInputText==0) wantInputText=1;
        if (wantInputText==2) {
            if (lastChar) {
                if (lastChar>=32) io.AddInputCharacter(lastChar);
                else {
                    if (lastChar==0x8) { //Backspace
                        io.AddKeyEvent(ImGuiKey_Backspace,true);
                        io.AddKeyEvent(ImGuiKey_Backspace,false);
                    } else if (lastChar==0xD) { //Return
                        io.AddKeyEvent(ImGuiKey_Enter,true);
                        io.AddKeyEvent(ImGuiKey_Enter,false);
                    }
                }
                lastChar=0;
            }
            if (move_cursorR) {
                if (move_cursorR==2) io.AddKeyEvent(ImGuiKey_RightArrow,true);
                else if (move_cursorR==1) io.AddKeyEvent(ImGuiKey_RightArrow,false);
                move_cursorR=0;
            }
            if (move_cursorL) {
                if (move_cursorL==2) io.AddKeyEvent(ImGuiKey_LeftArrow,true);
                else if (move_cursorL==1) io.AddKeyEvent(ImGuiKey_LeftArrow,false);
                move_cursorL=0;
            }
            if (shiftPressedL) {
                if (shiftPressedL==2) io.AddKeyEvent(ImGuiKey_LeftShift,true);
                else if (shiftPressedL==1) io.AddKeyEvent(ImGuiKey_LeftShift,false);
                shiftPressedL=0;
            }
            if (shiftPressedR) {
                if (shiftPressedR==2) io.AddKeyEvent(ImGuiKey_RightShift,true);
                else if (shiftPressedR==1) io.AddKeyEvent(ImGuiKey_RightShift,false);
                shiftPressedR=0;
            }
            if (keyDel) {
                if (keyDel==2) io.AddKeyEvent(ImGuiKey_Delete,true);
                else if (keyDel==1) io.AddKeyEvent(ImGuiKey_Delete,false);
                keyDel=0;
            }
        }
    } else wantInputText=0;
    
//    ImGuiKey key = ImGui_ImplIOS_KeyEventToImGuiKey();
//    static int cpt=0;
//    cpt++;
//    key=ImGuiKey_A;
//    if ((cpt%60)==0) {
//     
//        io.AddKeyEvent(key, 0);
//    }
//    if ((cpt%60)==10) {
//        io.AddKeyEvent(key, 1);
//    }
    
}

void ImGui_ImplIOS_ResetTapPos() {
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    io.WantCaptureMouse=true;
    
    currentEvent.event_type=IMGUI_IOS_Event_None;
    io.MousePos = ImVec2((float)0, (float)0);
    io.MouseDown[0] = 0;
}


void ImGui_ImplIOS_NewFrame(float w,float h,float scale,ImGuiIOSEvent *event)
{
    ImGuiIO &io = ImGui::GetIO();

    io.DisplaySize.x = w / scale;
    io.DisplaySize.y = h / scale ;
    io.DisplayFramebufferScale = ImVec2(scale, scale);
    
    io.DeltaTime = g_timer.GetElapsedSeconds();
    g_timer.Restart();
    
    ImGui_ImplIOS_UpdateEvent(event);
}

ImGuiKey ImGui_ImplIOS_KeyEventToImGuiKey()
{
//    int keycode='0';
//    switch (keycode)
//    {
//        case '0': return ImGuiKey_0;
//        case '1': return ImGuiKey_1;
//        default: break;
//    }
    return ImGuiKey_None;
}

@implementation ImGui_ImplIOS_UI

- (void)textFieldDidEndEditing:(UITextField *)textField {
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField{
    lastChar=0xD;
    [textField resignFirstResponder];
    return YES;
}

- (void)textFieldTextChanged:(UITextField *)textField {
    NSString *txt=textField.text;
    if ([txt length]>[self.text length]) {
        lastChar=[txt characterAtIndex:[txt length]-1];
    } else if ([txt length]<[self.text length]) {
        lastChar=8;
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
    
    move_cursorL=0;
    move_cursorR=0;
    shiftPressedL=0;
    shiftPressedR=0;
    lastChar=0;
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


@end

