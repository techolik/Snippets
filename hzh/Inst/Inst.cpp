// Inst.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Inst.h"
#include "CStringUtil.h"
#include <objidl.h>
#include <Gdiplus.h>
#include <Gdiplusheaders.h>
#include <gdiplusinit.h>
#include "CSysInfo.h"
#include "InstWnd.h"
#include "AppInst.h"

#define MAX_LOADSTRING 100

using namespace Gdiplus;

// Global Variables:

TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szPhone[MAX_LOADSTRING] = {0};
TCHAR szVerif[MAX_LOADSTRING] = {0};

POINT		s_mouseDownPos;

InstWnd*	g_InstWnd;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

unsigned __stdcall GetHidInfo(void *pParam)
{
	CAppInst::GetInstance().GetHid();
	CAppInst::GetInstance().PostInfo(__FILE__,__LINE__,"sancaster setup initial");
	return 0;
}


typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL(WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);
string VerifyWindowsVersion()
{
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;

	BOOL bOsVersionInfoEx;

	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*)&osvi);

	if (!bOsVersionInfoEx)
		return "fail to get OS version";

	pGNSI = (PGNSI)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
	if (NULL != pGNSI)
		pGNSI(&si);
	else
		GetSystemInfo(&si);

	if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && osvi.dwMajorVersion > 4)
	{
		if (osvi.wProductType == VER_NT_WORKSTATION && osvi.dwMajorVersion == 6)
		{
			if (osvi.dwMinorVersion >= 1 && osvi.dwMinorVersion <= 3)
			{
				return "";
			}
		}
	
	}

	return string("Platform id: ") + CStringUtil::IToStr(osvi.dwPlatformId)
		+ ", Major version: " + CStringUtil::IToStr(osvi.dwMajorVersion)
		+ ", Minor version: " + CStringUtil::IToStr(osvi.dwMinorVersion);
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	unsigned int nThreadID = 0;
	//操作系统版本
	string err = VerifyWindowsVersion();
	if (!err.empty())
	{
		MessageBox(NULL, L"很抱歉，不支持该操作系统", NULL, 0);
		CAppInst::GetInstance().PostError(__FILE__, __LINE__, string("Unsupported OS - ") + err);
		return 0;
	}
	CSysInfo::InitWSA();
	
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &GetHidInfo, (LPVOID)0, 0, &nThreadID);
	// Initialize global strings
	LoadString(hInstance, IDC_INST, szWindowClass, MAX_LOADSTRING);

	MyRegisterClass(hInstance);

	// Initialize GDI+.
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	MSG msg;
	g_InstWnd = new InstWnd(hInstance, szWindowClass);
	if (g_InstWnd->CreateMainWindow(nCmdShow))
	{
		// Main message loop:
		while (GetMessage(&msg, NULL, 0, 0))
		{
			//if (!IsDialogMessage(msg.hwnd, &msg)){
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			//}
		}
	}

	GdiplusShutdown(gdiplusToken);
	CAppInst::GetInstance().m_pWnd = g_InstWnd;
	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = {0};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hbrBackground	= NULL;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_INST));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_INST);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (message)
	{
		case WM_COMMAND:
			switch (wmEvent)
			{
				case BN_CLICKED:
					g_InstWnd->ButtonClicked(wmId);
					break;
				case EN_SETFOCUS:
					g_InstWnd->FocusIn(wmId);
					break;
				case EN_KILLFOCUS:
					g_InstWnd->FocusOut(wmId);
					break;
				case EN_CHANGE:
					g_InstWnd->TextChanged(wmId);
					break;
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
		
			break;
		case WM_CTLCOLOREDIT:
			g_InstWnd->SetColor((HDC)wParam, (HWND)lParam);
			break;
		case WM_DRAWITEM:
			g_InstWnd->DrawChildren(wmId, (LPDRAWITEMSTRUCT)lParam);
			break;
		case WM_PAINT:
			PAINTSTRUCT ps;
			HDC hdc;
			hdc = BeginPaint(hWnd, &ps);
		

			EndPaint(hWnd, &ps);
			break;
		case WM_NOTIFY:
			break;
		case WM_LBUTTONDOWN:
			s_mouseDownPos.x = (int)LOWORD(lParam);
			s_mouseDownPos.y = (int)HIWORD(lParam);
			break;
		case WM_MOUSEMOVE:
			if (wParam == MK_LBUTTON)
			{
				RECT mainWindowRect;
				GetWindowRect(hWnd, &mainWindowRect);
				int newx = mainWindowRect.left + (int)LOWORD(lParam) - s_mouseDownPos.x;
				int newy = mainWindowRect.top + (int)HIWORD(lParam) - s_mouseDownPos.y;
				int windowHeight = mainWindowRect.bottom - mainWindowRect.top;
				int windowWidth = mainWindowRect.right - mainWindowRect.left;

				SetWindowPos(hWnd, NULL, newx, newy, windowWidth, windowHeight, FALSE);
			}
			else{
				POINT pt = {(int)LOWORD(lParam), (int)HIWORD(lParam)};
				ClientToScreen(g_InstWnd->MainWnd(), &pt);
				g_InstWnd->MouseMove(pt, NULL);
			}
			break;
		case WM_ERASEBKGND:
			g_InstWnd->DrawBackground((HDC)wParam);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
