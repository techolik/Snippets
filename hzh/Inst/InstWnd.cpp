#include "stdafx.h"
#include "InstWnd.h"
#include "Resource.h"
#include "AppInst.h"
#include "CStringUtil.h"
#include "CFileUtil.h"

#include <objidl.h>
#include <Gdiplus.h>
#include <Gdiplusheaders.h>
#include <gdiplusinit.h>

#include <math.h>
#include <thread>

using namespace Gdiplus;
extern InstWnd*	g_InstWnd;

namespace{
	int		s_iconWidth = 35;
	int		s_nRoundCornerRadius = 2;
	int		s_nErrIconWidth = 22;
	int		s_nErrIconHeight = 26;
	COLORREF	s_colorInstallBackground = RGB(244, 96, 72);
	COLORREF	s_colorWhite = RGB(255, 255, 255);
	COLORREF	s_colorBlack = RGB(0, 0, 0);
	COLORREF	s_colorLightGray = RGB(160, 160, 160);
}

EditControl::EditControl(InstWnd* pParent)
	: m_pParent(pParent), m_bGood(true), m_bVerified(false), m_bFlushing(false)
{
	m_szText[0] = L'\0';
}

EditControl::~EditControl()
{
	DeleteObject(m_hGoodIcon);
	DeleteObject(m_hBadIcon);
	DeleteObject(m_hRightIcon);
	DeleteObject(m_hErrorLeftIcon);
	DeleteObject(m_hErrorRightIcon);
}

void EditControl::Initialize(int nPromptID, int nGoodIcon, int nBadIcon, int nRightIcon, int nErrorID)
{
	LoadString(m_pParent->Module(), nPromptID, m_szName, MAX_LOADSTRING);
	LoadString(m_pParent->Module(), nErrorID, m_szError, MAX_LOADSTRING);
	m_hGoodIcon = LoadBitmap(m_pParent->Module(), MAKEINTRESOURCE(nGoodIcon));
	m_hBadIcon = LoadBitmap(m_pParent->Module(), MAKEINTRESOURCE(nBadIcon));
	m_hRightIcon = LoadBitmap(m_pParent->Module(), MAKEINTRESOURCE(nRightIcon));
	m_hErrorLeftIcon = m_pParent->LoadBitMapFromPngResource(ID_PNG_ERROR_BALLOON_LEFT);
	m_hErrorRightIcon = m_pParent->LoadBitMapFromPngResource(ID_PNG_ERROR_BALLOON_RIGHT);
}

void EditControl::CreateControl(int nIconID, int nEditID, int nRightIconID, int nErrorBalloonID, int left, int top, int width, int height)
{
	m_nEditControlID = nEditID;

	// The icon is a static control
	m_hIcon = CreateWindow(
		L"STATIC", 	NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP,
		left + 5, top + 8, s_iconWidth, height,
		m_pParent->MainWnd(), (HMENU)nIconID, m_pParent->Module(), NULL);
	SendMessage(m_hIcon, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_hGoodIcon);

	// Then comes the text
	int text_left(left + s_iconWidth), text_width(width - 2 * s_iconWidth), text_height(28);
	m_hEdit = CreateWindow(
		L"EDIT", NULL, WS_VISIBLE | WS_CHILD | ES_LEFT | WS_TABSTOP,
		text_left, top + 12, text_width + 5, text_height,
		m_pParent->MainWnd(), (HMENU)nEditID, m_pParent->Module(), NULL);
	SendMessage(m_hEdit, WM_SETFONT, (WPARAM)m_pParent->DefaultFont(), 0);
	SetWindowText(m_hEdit, m_szName);

	// Intercept message for the edit
	FARPROC pOldProc = (FARPROC)SetWindowLongPtr(m_hEdit, GWL_WNDPROC, (DWORD)InstWnd::ChildrenWndProc);
	SetWindowLongPtr(m_hEdit, GWL_USERDATA, (LONG_PTR)pOldProc);

	// Then the icon for showing whether the text field is valid
	m_hRight = CreateWindow(
		L"STATIC", NULL, WS_CHILD | SS_BITMAP,
		text_left + text_width + 5, top + 8, s_iconWidth, height,
		m_pParent->MainWnd(), (HMENU)nRightIconID, m_pParent->Module(), NULL);
	SendMessage(m_hRight, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_hRightIcon);

	// The error balloon
	int nBalloonLenth = 2 * s_nErrIconWidth + 16 * lstrlen(m_szError);
	m_hErrorBalloon = CreateWindow(
		L"BUTTON", NULL, WS_CHILD | BS_OWNERDRAW,
		left - nBalloonLenth/* + 5*/, top + 7, nBalloonLenth, s_nErrIconHeight,
		m_pParent->MainWnd(), (HMENU)nErrorBalloonID, m_pParent->Module(), NULL);
	SendMessage(m_hErrorBalloon, WM_SETFONT, (WPARAM)m_pParent->DefaultFont(), 0);
}

void EditControl::FocusIn()
{
	// If the prompt string is showing, then it should disappear
	// when the edit has focus. Otherwise, the text should be 
	// selected
	if (IsPrompt())
		SetWindowText(m_hEdit, L"");
	else
		SendMessage(m_hEdit, EM_SETSEL, 0, -1);
}

void EditControl::FocusOut()
{
	TCHAR* szText = GetText();
	if (lstrlen(szText) == 0){
		// If the text is empty, then show back the prompt string
		SetWindowText(m_hEdit, m_szName);
	}
	else{
		Verify();
		m_bVerified = true;

		// 'Reset' the text to trigger update of the text color
		m_bFlushing = true;
		SetWindowText(m_hEdit, szText);
		m_bFlushing = false;
	}
}

COLORREF EditControl::GetColor()
{
	if (IsPrompt())
		return s_colorLightGray;

	// Only show bad color if it's verified to be not good, and it's just been verified.
	// If the text has been changed, then m_bVerified will be set false.
	if (!m_bGood && m_bVerified)
		return s_colorInstallBackground;

	return s_colorBlack;
}

void EditControl::TextChanged()
{
	// This is to update the icons when text has changed.
	if (!m_bFlushing){
		m_bVerified = false;
		if (!m_bGood){
			m_bGood = true;
			m_bFlushing = true;

			// Trigger redraw of text so to change color accordingly
			RECT r;
			GetClientRect(m_hEdit, &r);
			InvalidateRect(m_hEdit, &r, FALSE);
			Update();

			m_bFlushing = false;
		}
		ShowWindow(m_hRight, SW_HIDE);
	}
}

bool EditControl::Verify()
{
	if (m_bVerified)
		return m_bGood;

	// For now only verify code
	if (m_nEditControlID == IDC_CODE_EDIT){
		string strSetupCode = CStringUtil::WideStringToUtf8(GetText());
		string strUserId;
		string strEntName = CAppInst::GetInstance().GetEnterpriseName(strSetupCode, strUserId);
		if (strEntName == "ERROR1")
		{
			MessageBoxW(NULL, L"网络连接不上服务器", NULL, 0);
			CAppInst::GetInstance().PostError(__FILE__, __LINE__, "fail to connect to server");
			strEntName.clear();
		}
		// If the enterprise name can be retrived, then it's a good code.
		m_bGood = !strEntName.empty();
	}
#ifdef NDEBUG
	else if (lstrcmp(GetText(), L"bad") == 0){
		m_bGood = false;
	}
#endif
	else{
		m_bGood = true;
	}

	Update();
	return m_bGood;
}

void EditControl::Update()
{
	if (m_bGood){
		if (lstrlen(GetText()) && !IsPrompt())
			ShowWindow(m_hRight, SW_SHOW);
		ShowWindow(m_hErrorBalloon, SW_HIDE);
		SendMessage(m_hIcon, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_hGoodIcon);
	}
	else{
		ShowWindow(m_hRight, SW_HIDE);
		ShowWindow(m_hErrorBalloon, SW_SHOW);
		SendMessage(m_hIcon, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_hBadIcon);
	}
}

bool EditControl::IsPrompt()
{
	return lstrcmp(GetText(), m_szName) == 0;
}

TCHAR* EditControl::GetText()
{
	m_szText[0] = L'\0';
	GetWindowText(m_hEdit, m_szText, MAX_LOADSTRING - 1);
	return m_szText;
}

void EditControl::Paint(HDC hDC)
{
	// Paint the error balloon, the only thing that we need to manually paint
	//

	RECT r, w;
	GetClientRect(m_hErrorBalloon, &r);
	GetWindowRect(m_hErrorBalloon, &w);

	// First paint the background
	HDC hBackground = CreateCompatibleDC(hDC);
	SelectObject(hBackground, m_pParent->Background());
	POINT pt = {w.left, w.top};
	ScreenToClient(m_pParent->MainWnd(), &pt);
	BitBlt(hDC, 0, 0, r.right - r.left, r.bottom - r.top, hBackground, pt.x, pt.y, SRCCOPY);
	DeleteDC(hBackground);

	// Then alpha blend in the first part
	HDC hdcBitmap = CreateCompatibleDC(hDC);
	SelectObject(hdcBitmap, m_hErrorLeftIcon);
	BLENDFUNCTION bld;
	bld.AlphaFormat = AC_SRC_ALPHA;
	bld.BlendFlags = 0;
	bld.BlendOp = AC_SRC_OVER;
	bld.SourceConstantAlpha = 255;
	AlphaBlend(hDC, 0, 0, s_nErrIconWidth, s_nErrIconHeight, 
		hdcBitmap, 0, 0, s_nErrIconWidth, s_nErrIconHeight, bld);
	DeleteDC(hdcBitmap);
	
	// Fill the middle rectangle
	RECT midRect;
	midRect.left = s_nErrIconWidth;
	midRect.top = 0;
	midRect.right = r.right - s_nErrIconWidth;
	midRect.bottom = s_nErrIconHeight;
	HRGN rgn = CreateRectRgnIndirect(&midRect);
	HBRUSH bsh = CreateSolidBrush(s_colorInstallBackground);
	FillRgn(hDC, rgn, bsh);

	// Write the error message
	SetBkMode(hDC, TRANSPARENT);
	SetTextColor(hDC, s_colorWhite);
	DrawText(hDC, m_szError, lstrlen(m_szError), &midRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	// And alpha blend in the last part
	HDC hdcRBitmap = CreateCompatibleDC(hDC);
	SelectObject(hdcRBitmap, m_hErrorRightIcon);
	AlphaBlend(hDC, midRect.right, 0, s_nErrIconWidth, s_nErrIconHeight,
		hdcRBitmap, 0, 0, s_nErrIconWidth, s_nErrIconHeight, bld);
	DeleteDC(hdcRBitmap);
}

ButtonControl::ButtonControl(InstWnd* pParent)
	: m_pParent(pParent), m_bHover(false)
{

}

ButtonControl::~ButtonControl()
{
	DeleteObject(m_hIcon);
	DeleteObject(m_hHoverIcon);
	DeleteObject(m_hActiveIcon);
}

void ButtonControl::Initialize(int nIcon, int nHoverIcon, int nActiveIcon)
{
	m_hIcon = m_pParent->LoadBitMapFromPngResource(nIcon);
	m_hHoverIcon = m_pParent->LoadBitMapFromPngResource(nHoverIcon);
	m_hActiveIcon = m_pParent->LoadBitMapFromPngResource(nActiveIcon);
}

SIZE ButtonControl::Size()
{
	BITMAP iconInfo;
	GetObject(m_hIcon, sizeof(iconInfo), &iconInfo);
	SIZE sz = {iconInfo.bmWidth, iconInfo.bmHeight};
	return sz;
}

void ButtonControl::CreateControl(int nButtonID, int left, int top, int width, int height)
{
	m_hWnd = CreateWindow(
		L"BUTTON", NULL,
		WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
		left, top, width, height,
		m_pParent->MainWnd(), (HMENU)nButtonID, m_pParent->Module(), NULL);
	SendMessage(m_hWnd, WM_SETFONT, (WPARAM)m_pParent->DefaultFont(), 0);

	FARPROC pOldProc = (FARPROC)SetWindowLongPtr(m_hWnd, GWL_WNDPROC, (DWORD)InstWnd::ChildrenWndProc);
	SetWindowLongPtr(m_hWnd, GWL_USERDATA, (LONG_PTR)pOldProc);
}

void ButtonControl::ButtonClicked()
{
	//PAINTSTRUCT ps;
	//HDC hdc = BeginPaint(m_hWnd, &ps);
	////Paint(hdc, ODS_SELECTED); 
	//EndPaint(m_hWnd, &ps);
}

void ButtonControl::MouseMove(const POINT& pt, bool bOnButton)
{
	static HCURSOR hHand = LoadCursor(NULL, IDC_HAND);
	static HCURSOR hArrow = LoadCursor(NULL, IDC_ARROW);

	RECT r;
	GetClientRect(m_hWnd, &r);

	// Make a 5px area non-hot area so when mouse moves out we get to respond and update button
	RECT hotRect = r;
	hotRect.right -= 5;
	if (bOnButton && PtInRect(&hotRect, pt)){
		if (!m_bHover){
			m_bHover = true;
			InvalidateRect(m_hWnd, &r, false);
		}
		if (GetCursor() == hArrow)
			SetCursor(hHand);
	}
	else{
		if (m_bHover){
			m_bHover = false;
			InvalidateRect(m_hWnd, &r, false);
		}
		if (GetCursor() == hHand)
			SetCursor(hArrow);
	}
}

void ButtonControl::Paint(HDC hDC, int nState)
{
	HBITMAP hIcon = NULL;
	if (nState & ODS_SELECTED)
		hIcon = m_hActiveIcon;
	else if(m_bHover)
		hIcon = m_hHoverIcon;
	else
		hIcon = m_hIcon;

	RECT r, w;
	GetClientRect(m_hWnd, &r);
	GetWindowRect(m_hWnd, &w);

	// First paint the background
	HDC hBackground = CreateCompatibleDC(hDC);
	SelectObject(hBackground, m_pParent->Background());	
	POINT pt = {w.left, w.top};
	ScreenToClient(m_pParent->MainWnd(), &pt);
	BitBlt(hDC, 0, 0, r.right - r.left, r.bottom - r.top, hBackground, pt.x, pt.y, SRCCOPY);
	DeleteDC(hBackground);

	// Then alpha blend in the 'foreground'
	HDC hdcBitmap = CreateCompatibleDC(hDC);
	SelectObject(hdcBitmap, hIcon);
	BLENDFUNCTION bld;
	bld.AlphaFormat = AC_SRC_ALPHA;
	bld.BlendFlags = 0;
	bld.BlendOp = AC_SRC_OVER;
	bld.SourceConstantAlpha = 255;
	AlphaBlend(hDC, 0, 0, r.right - r.left, r.bottom - r.top, hdcBitmap, 0, 0, r.right - r.left, r.bottom - r.top, bld);
	DeleteDC(hdcBitmap);
}

ProgressControl::ProgressControl(InstWnd* pParent)
	: m_pParent(pParent), m_nRotationAngle(0), m_nProgress(0), m_nInterval(10), m_nTicks(0), m_nAlpha(255)
{
}

ProgressControl::~ProgressControl()
{
	delete m_pBackgroundIcon;
	DeleteObject(m_hProgressIcon);
	DeleteObject(m_hLargeFont);
	DeleteObject(m_hSmallFont);
	if (m_hTimer)
		KillTimer(NULL, m_hTimer);
}

void ProgressControl::Initialize(int nBackgroundIcon, int nProgressIcon)
{
	m_pBackgroundIcon = m_pParent->LoadGdiBitMapFromPngResource(nBackgroundIcon);
	m_hProgressIcon = m_pParent->LoadBitMapFromPngResource(nProgressIcon);

	LoadString(m_pParent->Module(), IDS_INSTALL_FINISH, m_szInstallFinish, MAX_LOADSTRING);

	HBITMAP hBackground;
	Color color;
	m_pBackgroundIcon->GetHBITMAP(color, &hBackground);
	GetObject(hBackground, sizeof(m_backgroundInfo), &m_backgroundInfo);
	GetObject(m_hProgressIcon, sizeof(m_progressInfo), &m_progressInfo);
	m_hLargeFont = CreateFont(36, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Calibri"));
	m_hSmallFont = CreateFont(20, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("宋体"));
}

extern LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
void ProgressControl::CreateControl()
{
	const TCHAR * c_szProgressClass = _T("ProgressControl");
	WNDCLASS wc = {0};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = g_InstWnd->Module();
	wc.lpszClassName = c_szProgressClass;
	RegisterClass(&wc);

	m_hWnd = CreateWindowEx(
		WS_EX_LAYERED, c_szProgressClass, NULL, WS_POPUP,
		0, 0, 0, 0, 
		m_pParent->MainWnd(), (HMENU)NULL, m_pParent->Module(), NULL);

	int left = (GetSystemMetrics(SM_CXSCREEN)) / 2 - (m_backgroundInfo.bmWidth / 2);
	int top = (GetSystemMetrics(SM_CYSCREEN)) / 2 - (m_backgroundInfo.bmHeight / 2);
	SetWindowPos(m_hWnd, HWND_TOPMOST, left, top, m_backgroundInfo.bmWidth, m_backgroundInfo.bmHeight, 0);
}

void ProgressControl::Update(int nProgress, int nSpeed)
{
	if (nProgress > 0){
		m_nTargetProgress = nProgress;
		m_nSpeed = nSpeed;
		m_nTicks = m_nSpeed / m_nInterval;
		m_nProgress++;
	}

	if (m_nTicks-- == 0 && m_nProgress < m_nTargetProgress){
		m_nProgress++;
		m_nTicks = m_nSpeed / m_nInterval;

		// Leave a 2 second pause before close
		if (m_nProgress == 100)
			m_nTicks = 250;
	}

	Update();

	// Leave 0.5 second interval to show "100%"
	if (m_nProgress == 100 && m_nTicks < 200)
		m_nProgress++;

	if (m_nProgress == 101)
	{
		// Fade out for the remaining 1 second
		if (m_nTicks < 100)
			m_nAlpha -= 2;

		// We're done
		if (m_nTicks == 0)
			SendMessage(m_hWnd, WM_CLOSE, 0, 0);
	}
}

void ProgressControl::Update()
{
	// Create the main memory DC
	HDC hMemoryDC = CreateCompatibleDC(NULL);

	// For some reason if we store the background HBITMAP, it will be modified
	// after the memory DC has been manipulated, so we have to retrieve it every time
	HBITMAP hBackgroundIcon;
	Color color;
	m_pBackgroundIcon->GetHBITMAP(color, &hBackgroundIcon);
	SelectObject(hMemoryDC, hBackgroundIcon);

	// Paint progress text
	if (m_nProgress > 0)
	{
		TCHAR szPercentage[10] = {0};
		swprintf_s((TCHAR*)szPercentage, 9, L"%d%%", m_nProgress);
		SetBkMode(hMemoryDC, TRANSPARENT);
		SetTextColor(hMemoryDC, s_colorWhite);
		RECT textRect;
		GetClientRect(m_hWnd, &textRect);
		textRect.top += 50;
		if (m_nProgress <= 100){
			SelectObject(hMemoryDC, m_hLargeFont);
			DrawText(hMemoryDC, szPercentage, lstrlen(szPercentage), &textRect, DT_CENTER | DT_SINGLELINE);
		}
		else{
			SelectObject(hMemoryDC, m_hSmallFont);
			DrawText(hMemoryDC, m_szInstallFinish, lstrlen(m_szInstallFinish), &textRect, DT_CENTER);
		}
	}

	// Create the transform
	m_nRotationAngle = (m_nRotationAngle + 5) % 360;
	const static double pi = std::acos(-1);
	double dAngle = (double)m_nRotationAngle / 180 * pi;
	XFORM xform;
	xform.eM11 = (float)std::cos(dAngle);
	xform.eM12 = (float)std::sin(dAngle);
	xform.eM21 = (float)-std::sin(dAngle);
	xform.eM22 = (float)std::cos(dAngle);
	xform.eDx = 0;
	xform.eDy = 0;

	// Set the transform on memory DC. 
	// Was trying to set it on source DC but AlphaBlend complains...
	SetGraphicsMode(hMemoryDC, GM_ADVANCED);
	SetWorldTransform(hMemoryDC, &xform);

	// Set origin of memory DC to its center, to facilitate AlphaBlend later
	SetViewportOrgEx(hMemoryDC, m_backgroundInfo.bmWidth / 2, m_backgroundInfo.bmHeight / 2, NULL);

	// Prepare the source DC containing the progress circle
	HDC hProgressBitMap = CreateCompatibleDC(NULL);
	SelectObject(hProgressBitMap, m_hProgressIcon);

	// Then blend...
	int extWidth = m_progressInfo.bmWidth / 2;
	int extHeight = m_progressInfo.bmHeight / 2;
	BLENDFUNCTION bld = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
	AlphaBlend(hMemoryDC, -extWidth, -extWidth, m_progressInfo.bmWidth, m_progressInfo.bmHeight,
		hProgressBitMap, 0, 0, m_progressInfo.bmWidth, m_progressInfo.bmHeight, bld);

	// Finally put the memory DC to screen
	HDC hDC = GetDC(NULL);
	POINT pt0 = {0};
	SIZE sz = {m_backgroundInfo.bmWidth, m_backgroundInfo.bmHeight};
	bld.SourceConstantAlpha = m_nAlpha;
	UpdateLayeredWindow(m_hWnd, hDC, NULL, &sz, hMemoryDC, &pt0, RGB(0, 0, 0), &bld, ULW_ALPHA);

	DeleteObject(hBackgroundIcon);
	DeleteDC(hMemoryDC);
	DeleteDC(hProgressBitMap);
	ReleaseDC(NULL, hDC);
}

void ProgressControl::Show()
{
	ShowWindow(m_hWnd, SW_SHOW);
	Update(0, 1000);
	m_hTimer = SetTimer(NULL, NULL, m_nInterval, (TIMERPROC)&UpdateTimerProc/*NULL*/);
}

void CALLBACK	ProgressControl::UpdateTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	g_InstWnd->UpdateProgress();
}

LRESULT CALLBACK ProgressControl::ProgressWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		default:
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

InstWnd::InstWnd(HINSTANCE hInst, TCHAR* szWindowClass)
:	m_hInst(hInst), 
	m_szWindowClass(szWindowClass),
	m_Name(this),
	m_Code(this),
	m_Close(this),
	m_Install(this),
	m_Progress(this),
	m_nLockedProgress(-1)
{
	m_nCtrlLeft = 310;
	m_nCtrlTop = 70;
	m_nVSpacing = 30;
}

InstWnd::~InstWnd()
{
	DeleteObject(m_hBackground);
}

bool InstWnd::CreateMainWindow(int nCmdShow)
{
	LoadResource();

	BITMAP bmpInfo;
	GetObject(m_hBackground, sizeof(bmpInfo), &bmpInfo);
	int left = (GetSystemMetrics(SM_CXSCREEN)) / 2 - (bmpInfo.bmWidth / 2);
	int top = (GetSystemMetrics(SM_CYSCREEN)) / 2 - (bmpInfo.bmHeight / 2);

	m_hWnd = CreateWindow(
		m_szWindowClass, m_szTitle, WS_POPUP,
		left, top, bmpInfo.bmWidth, bmpInfo.bmHeight, 
		NULL, NULL, Module(), NULL);
	if (!m_hWnd)
		return false;

	m_Name.Initialize(IDS_NAME, IDB_NAME, IDB_NAME_WRONG, IDB_RIGHT, IDS_NAME_ERROR);
	m_Code.Initialize(IDS_CODE, IDB_CODE, IDB_CODE_WRONG, IDB_RIGHT, IDS_CODE_ERROR);
	m_Close.Initialize(ID_PNG_CLOSE, ID_PNG_CLOSE_HOVER, ID_PNG_CLOSE_ACTIVE);
	m_Install.Initialize(ID_PNG_INSTALL, ID_PNG_INSTALL_HOVER, ID_PNG_INSTALL_ACTIVE);
	m_Progress.Initialize(ID_PNG_PROGRESS_BACKGROUND, ID_PNG_PROGRESS);

	CreateControls();

	ShowWindow(m_hWnd, nCmdShow);
	UpdateWindow(m_hWnd);

	return true;
}

void InstWnd::CreateControls()
{
	// Use install button size for the control size
	SIZE sz = m_Install.Size();
	m_nCtrlWidth = sz.cx;
	m_nCtrlHeight = sz.cy;

	int text_height(28), icon_width(35), top(m_nCtrlTop);

	m_Code.CreateControl(
		IDC_CODE_ICON, IDC_CODE_EDIT, IDB_RIGHT, IDC_CODE_ERROR_BALLOON,
		m_nCtrlLeft, top, m_nCtrlWidth, m_nCtrlHeight);

	top += m_nVSpacing + m_nCtrlHeight;
	m_Name.CreateControl(
		IDC_NAME_ICON, IDC_NAME_EDIT, IDB_RIGHT, IDC_NAME_ERROR_BALLOON,
		m_nCtrlLeft, top, m_nCtrlWidth, m_nCtrlHeight);

	top += m_nVSpacing + m_nCtrlHeight;
	m_Install.CreateControl(IDC_INSTALL_BUTTON,
		m_nCtrlLeft, top, m_nCtrlWidth, m_nCtrlHeight);

	SIZE szClose = m_Close.Size();
	RECT rect;
	GetWindowRect(m_hWnd, &rect);
	m_Close.CreateControl(IDC_CLOSE_BUTTON, rect.right - rect.left - szClose.cx, 4, szClose.cx, szClose.cy);

	m_Progress.CreateControl();
}

HBITMAP InstWnd::Background()
{
	return m_hBackground;
}

void InstWnd::DrawBackground(HDC hDC)
{
	HDC hdcBitmap = CreateCompatibleDC(hDC);
	SelectObject(hdcBitmap, m_hBackground);
	RECT r;
	GetClientRect(m_hWnd, &r);
	BitBlt(hDC, 0, 0, r.right - r.left, r.bottom - r.top, hdcBitmap, 0, 0, SRCCOPY);
	DeleteDC(hdcBitmap);

	// Draw rounded rect as background for the edit controls
	RECT rRR = {m_nCtrlLeft, m_nCtrlTop, m_nCtrlLeft + m_nCtrlWidth, m_nCtrlTop + m_nCtrlHeight};
	for (int i = 0; i < 2; i++){
		DrawRoundRect(m_hWnd, hDC, rRR, s_nRoundCornerRadius * 2, s_colorWhite);
		rRR.top += (m_nCtrlHeight + m_nVSpacing);
		rRR.bottom += (m_nCtrlHeight + m_nVSpacing);
	}
}

HFONT InstWnd::DefaultFont()
{
	static HFONT hFont = CreateFont(14, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("宋体"));
	return hFont;
}

void InstWnd::LoadResource()
{
	LoadString(Module(), IDS_APP_TITLE, m_szTitle, MAX_LOADSTRING);
	LoadString(Module(), IDS_INSTALL, m_szInstall, MAX_LOADSTRING);

	m_hBackground = LoadBitMapFromPngResource(ID_PNG_MAIN);
}

void InstWnd::DrawRect(HWND hWnd, const HDC& hDC, const RECT& r, const COLORREF& color)
{
	HBRUSH hBrush = CreateSolidBrush(color);
	HGDIOBJ hOldBrush = SelectObject(hDC, hBrush);
	HPEN hPen = CreatePen(PS_SOLID, 3, color);
	HGDIOBJ hOldPen = SelectObject(hDC, hPen);

	// Rectangle draws an outline by 1 px that we don't want, 
	// so reduce 1px from each side.
	Rectangle(hDC, r.left + 1, r.top + 1, r.right - 1, r.bottom - 1);

	SelectObject(hDC, hOldPen);
	SelectObject(hDC, hOldBrush);
	DeleteObject(hBrush);
	DeleteObject(hPen);
}

void InstWnd::DrawRoundRect(HWND hWnd, const HDC& hDC, const RECT& r, int diameter, const COLORREF& color)
{
	HBRUSH hBrush = CreateSolidBrush(color);
	HGDIOBJ hOldBrush = SelectObject(hDC, hBrush);
	HPEN hPen = CreatePen(PS_SOLID, 3, color);
	HGDIOBJ hOldPen = SelectObject(hDC, hPen);

	// RoundRect draws an outline by 1 px that we don't want, 
	// so reduce 1px from each side.
	RoundRect(hDC, r.left + 1, r.top + 1, r.right - 1, r.bottom - 1, diameter, diameter);

	SelectObject(hDC, hOldPen);
	SelectObject(hDC, hOldBrush);
	DeleteObject(hBrush);
	DeleteObject(hPen);
}

void InstWnd::SetColor(HDC hDC, HWND hWnd)
{
	if (hWnd == m_Name.Wnd())
		SetTextColor(hDC, m_Name.GetColor());
	else
		SetTextColor(hDC, m_Code.GetColor());
}

void InstWnd::DrawChildren(int nChildID, const LPDRAWITEMSTRUCT& lpDrawItem)
{
	if (nChildID == IDC_INSTALL_BUTTON)
		m_Install.Paint(lpDrawItem->hDC, lpDrawItem->itemState);
	else if (nChildID == IDC_CLOSE_BUTTON)
		m_Close.Paint(lpDrawItem->hDC, lpDrawItem->itemState);
	else if (nChildID == IDC_NAME_ERROR_BALLOON)
		m_Name.Paint(lpDrawItem->hDC);
	else if (nChildID == IDC_CODE_ERROR_BALLOON)
		m_Code.Paint(lpDrawItem->hDC);
}

void InstWnd::FocusIn(int nChildID)
{
	if (nChildID == IDC_NAME_EDIT)
		m_Name.FocusIn();
	else if (nChildID == IDC_CODE_EDIT)
		m_Code.FocusIn();
}

void InstWnd::FocusOut(int nChildID)
{
	if (nChildID == IDC_NAME_EDIT)
	{
		m_Name.FocusOut();
	}
	else
	{
		m_Code.FocusOut();	
	}
}

void InstWnd::TextChanged(int nChildID)
{
	if (nChildID == IDC_NAME_EDIT)
		m_Name.TextChanged();
	else
		m_Code.TextChanged();
}

void InstWnd::AsynchInstall()
{
	CAppInst::GetInstance().m_pWnd = g_InstWnd;
	int result = 0;
	if(result = CAppInst::GetInstance().Install())
	{
		SGUARD(&g_InstWnd->m_lockResult);
		g_InstWnd->m_nLockedResult = result;
	}
	CAppInst::GetInstance().PostInfo(__FILE__, __LINE__, string("setup finish with result:") + CStringUtil::IToStr(result));
}

void InstWnd::ButtonClicked(int nChildID)
{
	if (nChildID == IDC_CLOSE_BUTTON){
		m_Close.ButtonClicked();
		SendMessage(m_hWnd, WM_CLOSE, 0, 0);
	}
	else if (nChildID == IDC_INSTALL_BUTTON) {
		if (Verify())
		{
			string strName = CStringUtil::WideStringToAscii(m_Name.GetText());
			string strCode = CStringUtil::WideStringToAscii(m_Code.GetText());

			if (strCode.empty() == false && strName.empty() == false)
			{
				CAppInst::Status res = CAppInst::GetInstance().UserRegister(strCode, strName);
				if (CAppInst::Status::eOK == res)
				{
					// Kick off a thread for the install
					std::thread* t = new std::thread(&AsynchInstall);

					ShowWindow(MainWnd(), SW_HIDE);
					m_Progress.Show();
				}
				else
				{
					CAppInst::GetInstance().PostError(__FILE__, __LINE__, "register fail");
					MessageBoxW(NULL, L"注册用户失败", NULL, 0);
				}

			}
			else
			{
				MessageBoxW(NULL, L"输入内容为空", NULL, 0); 
			}

		}
	}
}

void InstWnd::TabPressed(HWND hWnd, bool bShiftDown)
{
	if (hWnd == m_Code.Wnd() && !bShiftDown)
		SetFocus(m_Name.Wnd());
	else if (hWnd == m_Name.Wnd() && bShiftDown)
		SetFocus(m_Code.Wnd());
}

void InstWnd::MouseMove(const POINT& pt, HWND hWnd)
{
	if (hWnd == m_Close.Wnd())
		m_Close.MouseMove(pt, true);
	else if (hWnd == m_Install.Wnd())
		m_Install.MouseMove(pt, true);
	else{
		m_Close.MouseMove(pt, false);
		m_Install.MouseMove(pt, false);
	}
}

void InstWnd::UpdateProgress(int nProgress , int nSpeed)
{
	SGUARD(&m_lockProgress);
	m_nLockedProgress = nProgress;
	m_nLockedSpeed = nSpeed;
}

void InstWnd::UpdateProgress()
{
	if (m_Progress.Wnd()) // For some reason, the first call from the dialog proc is too early
	{
		int nResult = 0;
		{
			SGUARD(&m_lockResult);
			nResult = m_nLockedResult;
		}
		int nProgress = -1;
		int nSpeed = 1000;
		{
			SGUARD(&m_lockProgress);
			nProgress = m_nLockedProgress;
			m_nLockedProgress = 0;
			nSpeed = m_nLockedSpeed;
		}
		m_Progress.Update(nProgress, nSpeed);

		if (nResult > 0){
			// Some error, report it
		}
	}
}

bool InstWnd::Verify()
{
	return m_Name.Verify() && m_Code.Verify();
}

HBITMAP InstWnd::LoadBitMapFromPngResource(int nID)
{
	HBITMAP hResult = NULL;
	if (Bitmap* pResult = LoadGdiBitMapFromPngResource(nID)){
		Color color;
		pResult->GetHBITMAP(color, &hResult);
		delete pResult;
	}
	return hResult;
}

Gdiplus::Bitmap* InstWnd::LoadGdiBitMapFromPngResource(int nID)
{
	Bitmap* pResult(NULL);
	if (HRSRC hResource = FindResource(Module(), MAKEINTRESOURCE(nID), RT_RCDATA))
	{
		if (HGLOBAL hResHandle = ::LoadResource(Module(), hResource))
		{
			const void* pResourceData = LockResource(hResHandle);
			DWORD imageSize = SizeofResource(Module(), hResource);

			if (HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize))
			{
				if (void* pBuffer = GlobalLock(hBuffer))
				{
					CopyMemory(pBuffer, pResourceData, imageSize);

					IStream* pStream = NULL;
					if (CreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK && pStream)
					{
						pResult = Gdiplus::Bitmap::FromStream(pStream);
						pStream->Release();
					}
					GlobalUnlock(pBuffer);
				}

				GlobalFree(hBuffer);
			}
		}
	}
	return pResult;
}

BOOL InstWnd::SaveZip(const string & strDir)
{
	
	if (HRSRC hResource = FindResource(Module(), MAKEINTRESOURCE(ID_ZIP_SANCASTER), RT_RCDATA))
	{
		if (HGLOBAL hResHandle = ::LoadResource(Module(), hResource))
		{
			const void* pResourceData = LockResource(hResHandle);
			DWORD imageSize = SizeofResource(Module(), hResource);

			if (HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize))
			{
				if (void* pBuffer = GlobalLock(hBuffer))
				{
					CopyMemory(pBuffer, pResourceData, imageSize);
					string strPath = strDir + "\\sancaster.zip";
					string strContent;
					strContent.assign((char*)pBuffer, imageSize);
					CFileUtil::SaveFile(strPath, strContent);
					GlobalUnlock(pBuffer);
				}
				GlobalFree(hBuffer);
			}
			else
				return FALSE;
		}
		else
			return FALSE;
	}
	else
		return FALSE;

	return TRUE;
}


BOOL InstWnd::IsOnlineInstall()
{
	if (HRSRC hResource = FindResource(Module(), MAKEINTRESOURCE(ID_ZIP_SANCASTER), RT_RCDATA))
	{
		if (HGLOBAL hResHandle = ::LoadResource(Module(), hResource))
		{
			const void* pResourceData = LockResource(hResHandle);
			DWORD imageSize = SizeofResource(Module(), hResource);
			if (imageSize < 1024)
			{
				return TRUE;
			}
		}
		else
			return TRUE;
	}
	else
		return TRUE;

	return FALSE;
}

bool s_bShiftDown = false;
LRESULT CALLBACK InstWnd::ChildrenWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_MOUSEMOVE:
		{
			POINT pt = {(int)LOWORD(lParam), (int)HIWORD(lParam)};
			g_InstWnd->MouseMove(pt, hWnd);
			break;
		}
		case WM_CHAR:
		{
			if (wParam == VK_TAB)
				g_InstWnd->TabPressed(hWnd, s_bShiftDown);
			break;
		}
		case WM_KEYDOWN:
		{
			if (wParam == VK_SHIFT)
				s_bShiftDown = true;
			if (wParam == VK_RETURN)
				g_InstWnd->ButtonClicked(IDC_INSTALL_BUTTON);
			break;
		}
		case WM_KEYUP:
		{
			if (wParam == VK_SHIFT)
				s_bShiftDown = false;
			break;
		}
		default:
			break;
	}

	// Our dirty little secret here - the old proc is set on the window as user data...
	//
	WNDPROC pOldProc = (WNDPROC)GetWindowLongPtr(hWnd, GWL_USERDATA);
	return CallWindowProc(pOldProc, hWnd, message, wParam, lParam);
}
