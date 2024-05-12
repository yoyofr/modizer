////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

//#include <windows.h>
#include "wsr_types.h" //YOYOFR

#include <stdio.h>
#include "input.h"
#include "log.h"
#include "oswan.h"
#include "io.h"
#include "config.h"
#include "ws.h"

//LPDIRECTINPUT8 lpDInput = NULL;
//LPDIRECTINPUTDEVICE8 lpKeyDevice = NULL;
//LPDIRECTINPUTDEVICE8 lpJoyDevice = NULL;
//DIJOYSTATE2 js;

//BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* lpddi, LPVOID lpContext)
//{
//	HRESULT hRet;
//
//	hRet = lpDInput->CreateDevice(lpddi->guidInstance, &lpJoyDevice, NULL);
//	if(FAILED(hRet))  return DIENUM_CONTINUE;
//
//	return DIENUM_STOP;
//}

int WsInputJoyInit(HWND hw)
{
//	HRESULT hRet;
//	if (lpJoyDevice != NULL)
//		return TRUE;
//
//	hRet = lpDInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, NULL, DIEDFL_ATTACHEDONLY);
//	if (hRet != DI_OK){
//		if (lpJoyDevice != NULL)
//		{
//			lpJoyDevice->Release();
//			lpJoyDevice = NULL;
//		}
//		return FALSE;
//	}
//	if (lpJoyDevice == NULL)
//	{
//		return FALSE;
//	}
//	hRet = lpJoyDevice->SetDataFormat(&c_dfDIJoystick2);
//	if (hRet != DI_OK){
//		if (lpJoyDevice != NULL)
//		{
//			lpJoyDevice->Release();
//			lpJoyDevice = NULL;
//		}
//		return FALSE;
//	}
//	hRet = lpJoyDevice->SetCooperativeLevel(hw, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
//	if (hRet != DI_OK){
//		if (lpJoyDevice != NULL)
//		{
//			lpJoyDevice->Release();
//			lpJoyDevice = NULL;
//		}
//		return FALSE;
//	}
//	lpJoyDevice->Acquire();
//
	return TRUE;
}

int WsInputInit(HWND hw)
{
//	DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&lpDInput, NULL);
//
//	lpDInput->CreateDevice(GUID_SysKeyboard, &lpKeyDevice, NULL);
//
//	lpKeyDevice->SetDataFormat(&c_dfDIKeyboard);
//
//	lpKeyDevice->SetCooperativeLevel(hw, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);

	return TRUE;
}

void WsInputJoyRelease(void)
{
//	if (lpJoyDevice != NULL)
//	{
//		lpJoyDevice->Unacquire();
//		lpJoyDevice->Release();
//		lpJoyDevice = NULL;
//	}
}

void WsInputRelease(void)
{
//	if (lpKeyDevice != NULL)
//	{
//		lpKeyDevice->Unacquire();
//		lpKeyDevice->Release();
//		lpKeyDevice = NULL;
//	}
//	if (lpDInput != NULL)
//	{
//		lpDInput->Release();
//		lpDInput = NULL;
//	}
}

BYTE checkJoystick(int value)
{
//	if ((value >= 1) && (value <= 32))
//	{
//		return((js.rgbButtons[value - 1] & 0x80) >> 7);
//	}
//	switch (value)
//	{
//	case WS_JOY_POV_UP:
//		if (js.rgdwPOV[0] == JOY_POVFORWARD) return 1;
//		if (js.rgdwPOV[0] == JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[0] == JOY_POVLEFT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV_RIGHT:
//		if (js.rgdwPOV[0] == JOY_POVRIGHT) return 1;
//		if (js.rgdwPOV[0] == JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[0] == JOY_POVRIGHT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV_DOWN:
//		if (js.rgdwPOV[0] == JOY_POVBACKWARD) return 1;
//		if (js.rgdwPOV[0] == JOY_POVRIGHT+JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[0] == JOY_POVBACKWARD+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV_LEFT:
//		if (js.rgdwPOV[0] == JOY_POVLEFT) return 1;
//		if (js.rgdwPOV[0] == JOY_POVBACKWARD+JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[0] == JOY_POVLEFT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV2_UP:
//		if (js.rgdwPOV[1] == JOY_POVFORWARD) return 1;
//		if (js.rgdwPOV[1] == JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[1] == JOY_POVLEFT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV2_RIGHT:
//		if (js.rgdwPOV[1] == JOY_POVRIGHT) return 1;
//		if (js.rgdwPOV[1] == JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[1] == JOY_POVRIGHT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV2_DOWN:
//		if (js.rgdwPOV[1] == JOY_POVBACKWARD) return 1;
//		if (js.rgdwPOV[1] == JOY_POVRIGHT+JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[1] == JOY_POVBACKWARD+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV2_LEFT:
//		if (js.rgdwPOV[1] == JOY_POVLEFT) return 1;
//		if (js.rgdwPOV[1] == JOY_POVBACKWARD+JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[1] == JOY_POVLEFT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV3_UP:
//		if (js.rgdwPOV[2] == JOY_POVFORWARD) return 1;
//		if (js.rgdwPOV[2] == JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[2] == JOY_POVLEFT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV3_RIGHT:
//		if (js.rgdwPOV[2] == JOY_POVRIGHT) return 1;
//		if (js.rgdwPOV[2] == JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[2] == JOY_POVRIGHT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV3_DOWN:
//		if (js.rgdwPOV[2] == JOY_POVBACKWARD) return 1;
//		if (js.rgdwPOV[2] == JOY_POVRIGHT+JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[2] == JOY_POVBACKWARD+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV3_LEFT:
//		if (js.rgdwPOV[2] == JOY_POVLEFT) return 1;
//		if (js.rgdwPOV[2] == JOY_POVBACKWARD+JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[2] == JOY_POVLEFT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV4_UP:
//		if (js.rgdwPOV[3] == JOY_POVFORWARD) return 1;
//		if (js.rgdwPOV[3] == JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[3] == JOY_POVLEFT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV4_RIGHT:
//		if (js.rgdwPOV[3] == JOY_POVRIGHT) return 1;
//		if (js.rgdwPOV[3] == JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[3] == JOY_POVRIGHT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV4_DOWN:
//		if (js.rgdwPOV[3] == JOY_POVBACKWARD) return 1;
//		if (js.rgdwPOV[3] == JOY_POVRIGHT+JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[3] == JOY_POVBACKWARD+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_POV4_LEFT:
//		if (js.rgdwPOV[3] == JOY_POVLEFT) return 1;
//		if (js.rgdwPOV[3] == JOY_POVBACKWARD+JOY_POVRIGHT/2) return 1;
//		if (js.rgdwPOV[3] == JOY_POVLEFT+JOY_POVRIGHT/2) return 1;
//		break;
//	case WS_JOY_AXIS_X_P:
//		if (js.lX > 0xC000) return 1;
//		break;
//	case WS_JOY_AXIS_X_M:
//		if (js.lX < 0x4000) return 1;
//		break;
//	case WS_JOY_AXIS_Y_P:
//		if (js.lY > 0xC000) return 1;
//		break;
//	case WS_JOY_AXIS_Y_M:
//		if (js.lY < 0x4000) return 1;
//		break;
//	case WS_JOY_AXIS_Z_P:
//		if (js.lZ > 0xC000) return 1;
//		break;
//	case WS_JOY_AXIS_Z_M:
//		if (js.lZ < 0x4000) return 1;
//		break;
//	case WS_JOY_AXIS_RX_P:
//		if (js.lRx > 0xC000) return 1;
//		break;
//	case WS_JOY_AXIS_RX_M:
//		if (js.lRx < 0x4000) return 1;
//		break;
//	case WS_JOY_AXIS_RY_P:
//		if (js.lRy > 0xC000) return 1;
//		break;
//	case WS_JOY_AXIS_RY_M:
//		if (js.lRy < 0x4000) return 1;
//		break;
//	case WS_JOY_AXIS_RZ_P:
//		if (js.lRz > 0xC000) return 1;
//		break;
//	case WS_JOY_AXIS_RZ_M:
//		if (js.lRz < 0x4000) return 1;
//		break;
//	case WS_JOY_SLIDER_P:
//		if (js.rglSlider[0] > 0xC000) return 1;
//		break;
//	case WS_JOY_SLIDER_M:
//		if (js.rglSlider[0] < 0x4000) return 1;
//		break;
//	case WS_JOY_SLIDER2_P:
//		if (js.rglSlider[1] > 0xC000) return 1;
//		break;
//	case WS_JOY_SLIDER2_M:
//		if (js.rglSlider[1] < 0x4000) return 1;
//		break;
//	}
	return 0;
}

int WsInputGetState(void)
{
//	HRESULT hRet;
//	BYTE diKeys[256];
//	ZeroMemory(&js, sizeof(DIJOYSTATE2));
//	ZeroMemory(diKeys, 256);
//
//	ws_key_start=0;
//	ws_key_left=0;
//	ws_key_right=0;
//	ws_key_up=0;
//	ws_key_down=0;
//	ws_key_left_y=0;
//	ws_key_right_y=0;
//	ws_key_up_y=0;
//	ws_key_down_y=0;
//	ws_key_button_1=0;
//	ws_key_button_2=0;
//	vsync=0;
//
//	if (lpJoyDevice != NULL)
//	{
//		hRet = lpJoyDevice->Poll();
//		if (FAILED(hRet))
//		{
//			hRet = lpJoyDevice->Acquire();
//			while(hRet == DIERR_INPUTLOST)
//				hRet = lpJoyDevice->Acquire();
//			return 0;
//		}
//
//		hRet = lpJoyDevice->GetDeviceState(sizeof(DIJOYSTATE2), &js);
//		if (hRet == DI_OK){
//			if(DrawMode & 1)
//			{
//				ws_key_up		= checkJoystick(joy_v_Xup);
//				ws_key_down		= checkJoystick(joy_v_Xdown);
//				ws_key_left		= checkJoystick(joy_v_Xleft);
//				ws_key_right	= checkJoystick(joy_v_Xright);
//				ws_key_up_y		= checkJoystick(joy_v_Yup);
//				ws_key_down_y	= checkJoystick(joy_v_Ydown);
//				ws_key_left_y	= checkJoystick(joy_v_Yleft);
//				ws_key_right_y	= checkJoystick(joy_v_Yright);
//				ws_key_start	= checkJoystick(joy_v_start);
//				ws_key_button_1	= checkJoystick(joy_v_A);
//				ws_key_button_2	= checkJoystick(joy_v_B);
//			}
//			else
//			{
//				ws_key_up		= checkJoystick(joy_h_Xup);
//				ws_key_down		= checkJoystick(joy_h_Xdown);
//				ws_key_left		= checkJoystick(joy_h_Xleft);
//				ws_key_right	= checkJoystick(joy_h_Xright);
//				ws_key_up_y		= checkJoystick(joy_h_Yup);
//				ws_key_down_y	= checkJoystick(joy_h_Ydown);
//				ws_key_left_y	= checkJoystick(joy_h_Yleft);
//				ws_key_right_y	= checkJoystick(joy_h_Yright);
//				ws_key_start	= checkJoystick(joy_h_start);
//				ws_key_button_1	= checkJoystick(joy_h_A);
//				ws_key_button_2	= checkJoystick(joy_h_B);
//			}
//			vsync	= checkJoystick(joy_vsync);
//		}
//	}
//
//	hRet = lpKeyDevice->Acquire();
//	if (hRet == DI_OK || hRet == S_FALSE)
//	{
//		hRet = lpKeyDevice->GetDeviceState( 256, diKeys );
//		if (hRet == DI_OK)
//		{
//			if(DrawMode & 1)
//			{
//				if (diKeys[key_v_Xup] & 0x80)		ws_key_up		= 1;
//				if (diKeys[key_v_Xdown] & 0x80)		ws_key_down		= 1;
//				if (diKeys[key_v_Xright] & 0x80)	ws_key_right	= 1;
//				if (diKeys[key_v_Xleft] & 0x80)		ws_key_left		= 1;
//				if (diKeys[key_v_Yup] & 0x80)		ws_key_up_y		= 1;
//				if (diKeys[key_v_Ydown] & 0x80)		ws_key_down_y	= 1;
//				if (diKeys[key_v_Yright] & 0x80)	ws_key_right_y	= 1;
//				if (diKeys[key_v_Yleft] & 0x80)		ws_key_left_y	= 1;
//				if (diKeys[key_v_start] & 0x80)		ws_key_start	= 1;
//				if (diKeys[key_v_A] & 0x80)			ws_key_button_1	= 1;
//				if (diKeys[key_v_B] & 0x80)			ws_key_button_2	= 1;
//			}
//			else
//			{
//				if (diKeys[key_h_Xup] & 0x80)		ws_key_up		= 1;
//				if (diKeys[key_h_Xdown] & 0x80)		ws_key_down		= 1;
//				if (diKeys[key_h_Xright] & 0x80)	ws_key_right	= 1;
//				if (diKeys[key_h_Xleft] & 0x80)		ws_key_left		= 1;
//				if (diKeys[key_h_Yup] & 0x80)		ws_key_up_y		= 1;
//				if (diKeys[key_h_Ydown] & 0x80)		ws_key_down_y	= 1;
//				if (diKeys[key_h_Yright] & 0x80)	ws_key_right_y	= 1;
//				if (diKeys[key_h_Yleft] & 0x80)		ws_key_left_y	= 1;
//				if (diKeys[key_h_start] & 0x80)		ws_key_start	= 1;
//				if (diKeys[key_h_A] & 0x80)			ws_key_button_1	= 1;
//				if (diKeys[key_h_B] & 0x80)			ws_key_button_2	= 1;
//			}
//			if (!vsync && (diKeys[key_vsync] & 0x80))			vsync	= 1;
//		}
//	}
//	return (
//		ws_key_start |
//		ws_key_left |
//		ws_key_right |
//		ws_key_up |
//		ws_key_down |
//		ws_key_left_y |
//		ws_key_right_y |
//		ws_key_up_y |
//		ws_key_down_y |
//		ws_key_button_1 |
//		ws_key_button_2
//	);
    return 0;
}
