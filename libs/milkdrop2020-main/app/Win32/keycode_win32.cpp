
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>



#include "keycode_win32.h"



KeyCode ConvertKeyCode_Win32(int code)
{
    switch (code)
    {
        case 'A'                    :
            return KEYCODE_A;
        case 'S'                    :
            return KEYCODE_S;
        case 'D'                    :
            return KEYCODE_D;
        case 'F'                    :
            return KEYCODE_F;
        case 'H'                    :
            return KEYCODE_H;
        case 'G'                    :
            return KEYCODE_G;
        case 'Z'                    :
            return KEYCODE_Z;
        case 'X'                    :
            return KEYCODE_X;
        case 'C'                    :
            return KEYCODE_C;
        case 'V'                    :
            return KEYCODE_V;
        case 'B'                    :
            return KEYCODE_B;
        case 'Q'                    :
            return KEYCODE_Q;
        case 'W'                    :
            return KEYCODE_W;
        case 'E'                    :
            return KEYCODE_E;
        case 'R'                    :
            return KEYCODE_R;
        case 'Y'                    :
            return KEYCODE_Y;
        case 'T'                    :
            return KEYCODE_T;
        case '1'                    :
            return KEYCODE_1;
        case '2'                    :
            return KEYCODE_2;
        case '3'                    :
            return KEYCODE_3;
        case '4'                    :
            return KEYCODE_4;
        case '6'                    :
            return KEYCODE_6;
        case '5'                    :
            return KEYCODE_5;
        case VK_OEM_NEC_EQUAL                :
            return KEYCODE_EQUALS;
        case '9'                    :
            return KEYCODE_9;
        case '7'                    :
            return KEYCODE_7;
        case VK_OEM_MINUS                :
            return KEYCODE_MINUS;
        case '8'                    :
            return KEYCODE_8;
        case '0'                    :
            return KEYCODE_0;
        case VK_OEM_6:
            return KEYCODE_RIGHTBRACKET;
        case 'O'                    :
            return KEYCODE_O;
        case 'U'                    :
            return KEYCODE_U;
        case VK_OEM_4          :
            return KEYCODE_LEFTBRACKET;
        case 'I'                    :
            return KEYCODE_I;
        case 'P'                    :
            return KEYCODE_P;
        case 'L'                    :
            return KEYCODE_L;
        case 'J'                    :
            return KEYCODE_J;
        case VK_OEM_7                :
            return KEYCODE_APOSTROPHE;
        case 'K'                    :
            return KEYCODE_K;
        case VK_OEM_1            :
            return KEYCODE_SEMICOLON;
        case VK_OEM_5            :
            return KEYCODE_BACKSLASH;
        case VK_OEM_COMMA:
            return KEYCODE_COMMA;
        case VK_OEM_2                :
            return KEYCODE_SLASH;
        case 'N'                    :
            return KEYCODE_N;
		case 'M' :
            return KEYCODE_M;
        case VK_OEM_PERIOD               :
            return KEYCODE_PERIOD;
		case VK_OEM_3                :
            return KEYCODE_GRAVE;
        case VK_DECIMAL        :
            return KEYCODE_KP_DECIMAL;
        case VK_MULTIPLY       :
            return KEYCODE_KP_MULTIPLY;
        case VK_OEM_PLUS           :
            return KEYCODE_KP_PLUS;
        case VK_CLEAR          :
            return KEYCODE_KP_CLEAR;
        case VK_DIVIDE         :
            return KEYCODE_KP_DIVIDE;
        //case VK_RE          :
          //  return KEYCODE_KP_ENTER;
       // case VK_OEM_MINUS          :
         //   return KEYCODE_KP_MINUS;
       // case VK_OEM_         :
         //   return KEYCODE_KP_EQUALS;
        case VK_NUMPAD0              :
            return KEYCODE_KP_0;
        case VK_NUMPAD1              :
            return KEYCODE_KP_1;
        case VK_NUMPAD2              :
            return KEYCODE_KP_2;
        case VK_NUMPAD3              :
            return KEYCODE_KP_3;
        case VK_NUMPAD4              :
            return KEYCODE_KP_4;
        case VK_NUMPAD5              :
            return KEYCODE_KP_5;
        case VK_NUMPAD6              :
            return KEYCODE_KP_6;
        case VK_NUMPAD7              :
            return KEYCODE_KP_7;
        case VK_NUMPAD8              :
            return KEYCODE_KP_8;
        case VK_NUMPAD9              :
            return KEYCODE_KP_9;
            
        case    VK_RETURN                  :
            return KEYCODE_RETURN;
        case    VK_TAB                     :
            return KEYCODE_TAB;
        case    VK_SPACE                   :
            return KEYCODE_SPACE;
        case    VK_DELETE                  :
            return KEYCODE_DELETE;
        case    VK_ESCAPE:
            return KEYCODE_ESCAPE;
		//case    VK_WIN:
		case    VK_LWIN                 :
            return KEYCODE_LGUI;
		case    VK_SHIFT:
		case    VK_LSHIFT:
            return KEYCODE_LSHIFT;
        case    VK_CAPITAL                :
            return KEYCODE_CAPSLOCK;
		case    VK_MENU:
		case    VK_LMENU                  :
            return KEYCODE_LALT;
		case    VK_CONTROL:
		case    VK_LCONTROL                 :
            return KEYCODE_LCTRL;
        case    VK_RSHIFT              :
            return KEYCODE_RSHIFT;
        case    VK_RMENU
			:
            return KEYCODE_RALT;
        case    VK_RCONTROL            :
            return KEYCODE_RCTRL;
     //   case                    :
       //     return KEYCODE_MODE;
        case    VK_F17                     :
            return KEYCODE_F17;
        case    VK_VOLUME_UP                :
            return KEYCODE_VOLUMEUP;
        case    VK_VOLUME_DOWN              :
            return KEYCODE_VOLUMEDOWN;
        case    VK_VOLUME_MUTE                    :
            return KEYCODE_MUTE;
        case    VK_F18                     :
            return KEYCODE_F18;
        case    VK_F19                     :
            return KEYCODE_F19;
        case    VK_F20                     :
            return KEYCODE_F20;
        case    VK_F5                      :
            return KEYCODE_F5;
        case    VK_F6                      :
            return KEYCODE_F6;
        case    VK_F7                      :
            return KEYCODE_F7;
        case    VK_F3                      :
            return KEYCODE_F3;
        case    VK_F8                      :
            return KEYCODE_F8;
        case    VK_F9                      :
            return KEYCODE_F9;
        case    VK_F11                     :
            return KEYCODE_F11;
        case    VK_F13                     :
            return KEYCODE_F13;
        case    VK_F16                     :
            return KEYCODE_F16;
        case    VK_F14                     :
            return KEYCODE_F14;
        case    VK_F10                     :
            return KEYCODE_F10;
        case    VK_F12                     :
            return KEYCODE_F12;
        case    VK_F15                     :
            return KEYCODE_F15;
        case    VK_HELP                    :
            return KEYCODE_HELP;
        case    VK_HOME                    :
            return KEYCODE_HOME;
        case    VK_PRIOR                  :
            return KEYCODE_PAGEUP;
        case    VK_BACK           :
            return KEYCODE_DELETE;
        case    VK_F4                      :
            return KEYCODE_F4;
        case    VK_END                     :
            return KEYCODE_END;
        case    VK_F2                      :
            return KEYCODE_F2;
        case    VK_NEXT                :
            return KEYCODE_PAGEDOWN;
        case    VK_F1                      :
            return KEYCODE_F1;
        case    VK_LEFT               :
            return KEYCODE_LEFT;
        case    VK_RIGHT              :
            return KEYCODE_RIGHT;
        case    VK_DOWN               :
            return KEYCODE_DOWN;
        case    VK_UP                 :
            return KEYCODE_UP;
            
        default:
            return KEYCODE_UNKNOWN;
    }
    
}
