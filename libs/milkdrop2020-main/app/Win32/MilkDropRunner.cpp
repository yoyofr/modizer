// MilkDropRunner.cpp : Defines the entry point for the application.
//


#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <ShellScalingApi.h>

#pragma comment(lib,"Shcore.lib")

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <stdio.h>
#include "MilkDropRunner.h"

#include "keycode_win32.h"

#include "VizController.h"
#include "../external/imgui/imgui.h"
#include "../external/imgui/backends/imgui_impl_win32.h"

#include "imgui_support.h"

#define CLASSNAME "MilkDropPortable"


using namespace render;

std::string GetPluginDirectory()
{
	HMODULE hModule = NULL;
/*	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&GetPluginDirectory,
		&hModule);
		*/

	char buf[MAX_PATH];
	GetModuleFileName(hModule, buf, MAX_PATH);
	char *p = buf + strlen(buf);
	while (p >= buf && *p != '\\') p--;
	if (++p >= buf) *p = 0;
	return buf;
}


//
// dx window implementation
//
class DXWindow 
{
public:
	DXWindow(HWND hwnd);
	virtual ~DXWindow();

	virtual void SetCaption(std::string caption);
	virtual void Show();
	virtual void Hide();

	virtual HWND GetHWND() { return m_hwnd; }

	virtual bool WasClosed()
	{
		return m_hwnd == NULL;
	}


	void RenderFrame()
	{

	}



	HWND GetHandle() { return m_hwnd; }

	LRESULT WindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);


	VizControllerPtr m_controller;
private:
	HWND m_hwnd;
};
using DXWindowPtr = std::shared_ptr<DXWindow>;



static LRESULT CALLBACK WindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam)
{

	DXWindow* p = (DXWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (p)
	{
		return p->WindowProc(hWnd, uMsg, wParam, lParam);
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}


DXWindowPtr DXContextCreateWindow()
{
	HMODULE hInstance = GetModuleHandle(NULL);

	// Set up and register window class
	WNDCLASS wc = { 0 };
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; // CS_DBLCLKS lets the window receive WM_LBUTTONDBLCLK, for toggling fullscreen mode...
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.cbWndExtra = sizeof(DWORD);
	wc.hInstance = hInstance;
	wc.hIcon = NULL; // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PLUGIN_ICON));//NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL; // (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = CLASSNAME;
	ATOM classAtom = RegisterClass(&wc);

	int width = 1024;
	int height = 1024;
	HWND hwnd = CreateWindowEx(
		0, // extended style
		MAKEINTATOM(classAtom), // class
		CLASSNAME, // caption
		WS_OVERLAPPEDWINDOW, // style
		CW_USEDEFAULT, // left
		CW_USEDEFAULT, // top
		//width, height,
		CW_USEDEFAULT,  // temporary width
		CW_USEDEFAULT,  // temporary height
		NULL,
		NULL, // menu
		hInstance, // instance
		NULL
	); // parms

	if (!hwnd)
	{
		LogError("DXContextCreateWindow failed");
		return NULL;
	}

	DXWindowPtr window = std::make_shared<DXWindow>(hwnd);
	window->Show();
	window->SetCaption(CLASSNAME);
	return window;
}

DXWindow::DXWindow(HWND hwnd)
{
	m_hwnd = hwnd;
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

DXWindow::~DXWindow()
{
	DestroyWindow(m_hwnd);
}


void DXWindow::SetCaption(std::string caption)
{
	SetWindowText(m_hwnd, caption.c_str());
}

void DXWindow::Show()
{
	SetFocus(m_hwnd);
	SetActiveWindow(m_hwnd);
	SetForegroundWindow(m_hwnd);
	ShowWindow(m_hwnd, SW_NORMAL);
}

void DXWindow::Hide()
{
	ShowWindow(m_hwnd, SW_HIDE);
}





IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT DXWindow::WindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	
	switch (uMsg)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		bool down = (uMsg == WM_KEYDOWN);
		int modifiers = HIWORD(lParam);

		ImGuiIO& io = ImGui::GetIO();

		//e.c = (char)wParam;
		auto code =  ConvertKeyCode_Win32((int)wParam );

		io.KeysDown[code] = down;
		io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
		io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
		io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
		//io.Key = (::GetKeyState(VK_LWIN) & 0x8000) != 0;

/*
		e.KeyCtrl = modifiers & MOD_CONTROL;
		e.KeyCommand = modifiers & MOD_WIN;
		e.KeyAlt = modifiers & MOD_ALT;
		e.KeyShift = modifiers & MOD_SHIFT;
		*/

		

		break;
	}

	case WM_DESTROY:
		m_hwnd = NULL;
		break;

	case WM_LBUTTONDOWN:
		break;

	case WM_LBUTTONDBLCLK:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);

}


namespace render {
	namespace d3d9 {
		extern ContextPtr D3D9CreateContext(HWND hWnd);
	}
}


namespace render {
	namespace d3d11 {
		extern ContextPtr D3D11CreateContext(HWND hWnd);
	}
}


void RedirectIOToConsole()
{
	//CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE* fp;

	// allocate a console for this app
	AllocConsole();

	freopen_s(&fp, "conout$", "w", stdout);
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);

	
	freopen_s(&fp, "conout$", "w", stderr);
	*stderr = *fp;
	setvbuf(stderr, NULL, _IONBF, 0);


	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
	// point to console as well
	std::ios::sync_with_stdio();
}


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

#if DEBUG
	//_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_EVERY_128_DF | _CRTDBG_LEAK_CHECK_DF);
//	_CrtSetBreakAlloc(8488);
#endif

//	::SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	RedirectIOToConsole();

	// create app window
	DXWindowPtr window = DXContextCreateWindow();




	HWND hwnd = (HWND)window->GetHWND();

	ContextPtr context;

#if 1
	context = render::d3d9::D3D9CreateContext(hwnd);
#else
	context = render::d3d11::D3D11CreateContext(hwnd);
#endif

	char dir[MAX_PATH];
	GetCurrentDirectoryA(sizeof(dir), dir);
	LogPrint("Current Directory: %s\n", dir);

	std::string assetDir = dir; //GetPluginDirectory();
	assetDir += "\\..\\..\\assets";

	assetDir = PathGetFullPath(assetDir);

    std::string userDir = getenv("HOMEPATH");

	userDir = PathCombine(userDir, "MilkdropPortable");
	DirectoryCreate(userDir)
		;
	LogPrint("Asset Directory: %s\n", assetDir.c_str());
	LogPrint("User Directory: %s\n", userDir.c_str());

 

	VizControllerPtr vc = CreateVizController(context, assetDir, userDir);
	window->m_controller = vc;
	ImGui_ImplWin32_Init(hwnd);

	MSG msg;
	while(!vc->ShouldQuit())
	{
		PROFILE_FRAME()

		// Check to see if any messages are waiting in the queue
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Translate the message and dispatch it to WindowProc()
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If the message is WM_QUIT, exit the while loop
		if(msg.message == WM_QUIT)
			break;

		if (window->WasClosed())
		{
			break;
		}

		ImGui_ImplWin32_NewFrame();

		context->BeginScene();
		vc->Render(0, 1);
		context->EndScene();
		context->Present();
	}

	vc = nullptr;
	return (int) msg.wParam;
}


