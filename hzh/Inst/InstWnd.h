#pragma once
#include <Windows.h>
#include "CBaseLibInc.h"
#include "CReadWriteLock.h"

#define MAX_LOADSTRING 100

class InstWnd;
class EditControl
{
public:
	EditControl(InstWnd* pParent);
	~EditControl();

	void		Initialize(int nPromptID, int nGoodIcon, int nBadIcon, int nRightIcon, int nErrorStr);
	void		CreateControl(int nIconID, int nEditID, int nRightIconID, int nErrorBalloonID,
		int left, int top, int width, int height);

	void		FocusIn();
	void		FocusOut();
	void		TextChanged();
	void		Update();
	COLORREF	GetColor();
	HWND		Wnd() { return m_hEdit; }
	bool		Verify();
	bool		IsPrompt();
	TCHAR*		GetText();
	void		Paint(HDC hDC);

private:
	InstWnd*	m_pParent;
	int			m_nEditControlID;
	HWND		m_hIcon;
	HWND		m_hEdit;
	HWND		m_hRight;
	HWND		m_hErrorBalloon;
	HBITMAP		m_hGoodIcon;
	HBITMAP		m_hBadIcon;
	HBITMAP		m_hRightIcon;
	HBITMAP		m_hErrorLeftIcon;
	HBITMAP		m_hErrorRightIcon;
	TCHAR		m_szName[MAX_LOADSTRING];
	TCHAR		m_szError[MAX_LOADSTRING];
	TCHAR       m_szText[MAX_LOADSTRING];
	bool		m_bGood;
	bool		m_bVerified;
	bool		m_bFlushing;
};

class ButtonControl
{
public:
	ButtonControl(InstWnd* pParent);
	~ButtonControl();

	void		Initialize(int nIcon, int nHoverIcon, int nActiveIcon);
	void		CreateControl(int nButtonID, int left, int top, int width, int height);
	SIZE		Size();

	void		ButtonClicked();
	void		MouseMove(const POINT& pt, bool bOnButton);
	void		Paint(HDC hDC, int nState);
	HWND		Wnd() { return m_hWnd; }

private:
	InstWnd*	m_pParent;
	bool		m_bHover;
	HWND		m_hWnd;
	HBITMAP		m_hIcon;
	HBITMAP		m_hActiveIcon;
	HBITMAP		m_hHoverIcon;
};

namespace Gdiplus{
	class Bitmap;
}

class ProgressControl
{
public:
	ProgressControl(InstWnd* pParent);
	~ProgressControl();

	void		Initialize(int nBackgroundIcon, int nProgressIcon);
	void		CreateControl();

	HWND		Wnd() { return m_hWnd; }
	void		Update(int nProgress, int nSpeed);
	void		Show();

	static void	CALLBACK	UpdateTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static LRESULT CALLBACK ProgressWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	void		Update();
	InstWnd*	m_pParent;
	UINT_PTR	m_hTimer;
	HWND		m_hWnd;
	HFONT		m_hLargeFont;
	HFONT		m_hSmallFont;
	BITMAP		m_backgroundInfo;
	BITMAP		m_progressInfo;
	TCHAR		m_szInstallFinish[MAX_LOADSTRING];
	Gdiplus::Bitmap*		m_pBackgroundIcon;
	HBITMAP		m_hProgressIcon;
	int			m_nRotationAngle;
	int			m_nInterval;
	int			m_nProgress;
	int			m_nTargetProgress;
	int			m_nSpeed;
	int			m_nTicks;
	int			m_nAlpha;
};

class InstWnd
{
public:
	InstWnd(HINSTANCE hInst, TCHAR*	szWindowClass);
	~InstWnd();

	bool		CreateMainWindow(int nCmdShow);
	void		CreateControls();

	void		DrawBackground(HDC hDC);
	void		DrawRect(HWND hWnd, const HDC& hDC, const RECT& r, const COLORREF& color);
	void		DrawRoundRect(HWND hWnd, const HDC& hDC, const RECT& r, int diameter, const COLORREF& color);
	void		SetColor(HDC hDC, HWND hWnd);
	void		DrawChildren(int nChildID, const LPDRAWITEMSTRUCT& lpDrawItem);
	void		FocusIn(int nChildID);
	void		FocusOut(int nChildID);
	void		TextChanged(int nChildID);
	void		ButtonClicked(int nChildID);
	void		TabPressed(HWND hWnd, bool bShiftDown);
	void		MouseMove(const POINT& pt, HWND hWnd);
	bool		Verify();

	HINSTANCE	Module()	{ return m_hInst; }
	HWND		MainWnd()	{ return m_hWnd; }
	HFONT		DefaultFont();
	HBITMAP		LoadBitMapFromPngResource(int nID);
	Gdiplus::Bitmap*		LoadGdiBitMapFromPngResource(int nID);
	HBITMAP		Background();
	BOOL		SaveZip(const string & strDir);
	BOOL        IsOnlineInstall();

	static LRESULT CALLBACK ChildrenWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static void	AsynchInstall();

	// Call this from the install thread
	void		UpdateProgress(int nProgress, int nSpeed);

	// Call this from the main thread timer
	void		UpdateProgress();

private:
	void		LoadResource();

	int			m_nCtrlLeft;
	int			m_nCtrlTop;
	int			m_nCtrlWidth;
	int			m_nCtrlHeight;
	int			m_nVSpacing;

	HINSTANCE	m_hInst;
	HWND		m_hWnd;
	HBITMAP		m_hBackground;

	TCHAR		m_szTitle[MAX_LOADSTRING];
	TCHAR		m_szInstall[MAX_LOADSTRING];
	TCHAR*		m_szWindowClass;

	EditControl		m_Name;
	EditControl		m_Code;
	ButtonControl	m_Close;
	ButtonControl	m_Install;
	ProgressControl m_Progress;

	CLock			m_lockProgress;
	int				m_nLockedProgress;
	int				m_nLockedSpeed;
	CLock			m_lockResult;
	int				m_nLockedResult;
};
