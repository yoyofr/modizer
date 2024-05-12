/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __CONFIG_H__
#define __CONFIG_H__

/////////////////////////////////////////////////////////////////////////////////////////////////
// defines
/////////////////////////////////////////////////////////////////////////////////////////////////
#define WS_JOY_BUTTON_1   1
#define WS_JOY_BUTTON_2   2
#define WS_JOY_BUTTON_3   3
#define WS_JOY_BUTTON_4   4
#define WS_JOY_BUTTON_5   5
#define WS_JOY_BUTTON_6   6
#define WS_JOY_BUTTON_7   7
#define WS_JOY_BUTTON_8   8
#define WS_JOY_POV_UP     0x101
#define WS_JOY_POV_RIGHT  0x102
#define WS_JOY_POV_DOWN   0x104
#define WS_JOY_POV_LEFT   0x108
#define WS_JOY_POV2_UP    0x111
#define WS_JOY_POV2_RIGHT 0x112
#define WS_JOY_POV2_DOWN  0x114
#define WS_JOY_POV2_LEFT  0x118
#define WS_JOY_POV3_UP    0x121
#define WS_JOY_POV3_RIGHT 0x122
#define WS_JOY_POV3_DOWN  0x124
#define WS_JOY_POV3_LEFT  0x128
#define WS_JOY_POV4_UP    0x131
#define WS_JOY_POV4_RIGHT 0x132
#define WS_JOY_POV4_DOWN  0x134
#define WS_JOY_POV4_LEFT  0x138
#define WS_JOY_AXIS_X_P   0x1000
#define WS_JOY_AXIS_X_M   0x1001
#define WS_JOY_AXIS_Y_P   0x1002
#define WS_JOY_AXIS_Y_M   0x1003
#define WS_JOY_AXIS_Z_P   0x1004
#define WS_JOY_AXIS_Z_M   0x1005
#define WS_JOY_AXIS_RX_P  0x1006
#define WS_JOY_AXIS_RX_M  0x1007
#define WS_JOY_AXIS_RY_P  0x1008
#define WS_JOY_AXIS_RY_M  0x1009
#define WS_JOY_AXIS_RZ_P  0x100A
#define WS_JOY_AXIS_RZ_M  0x100B
#define WS_JOY_SLIDER_P   0x1100
#define WS_JOY_SLIDER_M   0x1101
#define WS_JOY_SLIDER2_P  0x1102
#define WS_JOY_SLIDER2_M  0x1103

/////////////////////////////////////////////////////////////////////////////////////////////////
// variables
/////////////////////////////////////////////////////////////////////////////////////////////////
extern int key_h_Xup;
extern int key_h_Xdown;
extern int key_h_Xright;
extern int key_h_Xleft;
extern int key_h_Yup;
extern int key_h_Ydown;
extern int key_h_Yright;
extern int key_h_Yleft;
extern int key_h_start;
extern int key_h_A;
extern int key_h_B;
extern int key_v_Xup;
extern int key_v_Xdown;
extern int key_v_Xright;
extern int key_v_Xleft;
extern int key_v_Yup;
extern int key_v_Ydown;
extern int key_v_Yright;
extern int key_v_Yleft;
extern int key_v_start;
extern int key_v_A;
extern int key_v_B;
extern int key_vsync;

extern int joy_h_Xup;
extern int joy_h_Xdown;
extern int joy_h_Xright;
extern int joy_h_Xleft;
extern int joy_h_Yup;
extern int joy_h_Ydown;
extern int joy_h_Yright;
extern int joy_h_Yleft;
extern int joy_h_start;
extern int joy_h_A;
extern int joy_h_B;
extern int joy_v_Xup;
extern int joy_v_Xdown;
extern int joy_v_Xright;
extern int joy_v_Xleft;
extern int joy_v_Yup;
extern int joy_v_Ydown;
extern int joy_v_Yright;
extern int joy_v_Yleft;
extern int joy_v_start;
extern int joy_v_A;
extern int joy_v_B;
extern int joy_vsync;

/////////////////////////////////////////////////////////////////////////////////////////////////
// functions
/////////////////////////////////////////////////////////////////////////////////////////////////
//void conf_loadIniFile(void);
//void conf_saveIniFile(void);

//LRESULT CALLBACK GuiConfigKeyboardProc(HWND hDlgWnd, UINT message, WPARAM wParam, LPARAM lParam);
//LRESULT CALLBACK GuiConfigJoypadProc(HWND hDlgWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif
