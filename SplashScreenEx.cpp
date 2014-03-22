#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <cassert>

#include "SplashScreenEx.h"

#ifndef AW_HIDE
	#define AW_HIDE 0x00010000
	#define AW_BLEND 0x00080000
#endif

#ifndef CS_DROPSHADOW
	#define CS_DROPSHADOW   0x00020000
#endif

#define ID_TIMER2 2


BOOL SplashScreen::SetBitmap(HBITMAP bmp, short red, short green, short blue)
{
    if (!bmp)
        return FALSE;

    DeleteObject(_bitmap);
    _bitmap = bmp;

    BITMAP bm;
	GetObject(_bitmap, sizeof(bm), &bm);

	_bitmapWidth = bm.bmWidth;
	_bitmapHeight = bm.bmHeight;
	SetRect(&_rcText, 0,0, _bitmapWidth, _bitmapHeight);

	if (_style & CSS_CENTERSCREEN)
	{
		_xPos = (GetSystemMetrics(SM_CXFULLSCREEN) - _bitmapWidth)/2;
		_yPos = (GetSystemMetrics(SM_CYFULLSCREEN) - _bitmapHeight)/2;
	}
	else if (_style & CSS_CENTERAPP)
	{
	    if (_hwndParent == NULL)
		    return FALSE;

		RECT parentRect;
		GetWindowRect(_hwndParent, &parentRect);

		_xPos = parentRect.left + (parentRect.right - parentRect.left - _bitmapWidth)/2;
		_yPos = parentRect.top + (parentRect.bottom - parentRect.top - _bitmapHeight)/2;
	}

	if (red != -1 && green != -1 && blue != -1)
	{
		_region = CreateRgnFromBitmap(_bitmap, RGB(red,green,blue));
		SetWindowRgn(_splashScreen, _region, TRUE);
	}

	return TRUE;
}
BOOL SplashScreen::SetBitmap(UINT bitmapID, short red, short green, short blue)
{
	HBITMAP bmp = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(bitmapID));
	return SetBitmap(bmp, red, green, blue);
}
BOOL SplashScreen::SetBitmap(LPCTSTR fileName, short red, short green, short blue)
{
	HBITMAP bmp = (HBITMAP)LoadImage(GetModuleHandle(NULL), fileName, IMAGE_BITMAP,
        0,0, LR_LOADFROMFILE);
	return SetBitmap(bmp, red, green, blue);
}

void SplashScreen::SetTextFont(LPCTSTR fontName, int height, int style)
{
	_customFont = CreateFont(height, 0, 0, 0, FW_NORMAL,FALSE, FALSE, 0, ANSI_CHARSET,
   		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
   		DEFAULT_PITCH | FF_SWISS, fontName);

    LOGFONT lf;
	GetObject(_customFont, sizeof(LOGFONT), &lf);

	if (style & CSS_TEXT_BOLD)
		lf.lfWeight = FW_BOLD;
	else
		lf.lfWeight = FW_NORMAL;

	if (style & CSS_TEXT_ITALIC)
		lf.lfItalic=TRUE;
	else
		lf.lfItalic=FALSE;

	if (style & CSS_TEXT_UNDERLINE)
		lf.lfUnderline=TRUE;
	else
		lf.lfUnderline=FALSE;

	DeleteObject(_customFont);
	_customFont = CreateFontIndirect(&lf);
}
void SplashScreen::SetDefaultFont()
{
	_customFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}
void SplashScreen::SetText(LPCTSTR text)
{
	_text = text;
    Invalidate();
}
void SplashScreen::SetTextColor(COLORREF textColor)
{
	_textColor = textColor;
	Invalidate();
}
void SplashScreen::SetTextRect(LPRECT rcText)
{
	_rcText = *rcText;
	Invalidate();
}
void SplashScreen::SetTextRect(int x, int y, int width, int height)
{
    RECT rcText = {x, y, width, height};
    SetTextRect(&rcText);
}
void SplashScreen::SetTextFormat(UINT textFormat)
{
	_textFormat = textFormat;
}

SplashScreen::SplashScreen()
{
    _hwndParent = NULL;
	_text = _T("");
	_region = NULL;
	_bitmapWidth = 0;
	_bitmapHeight = 0;
	_xPos = 0;
	_yPos = 0;
	_timeout = 2000;
	_style = 0;
	SetRect(&_rcText, 0,0,0,0);
	_textColor = RGB(0,0,0);
	_textFormat = DT_CENTER | DT_VCENTER | DT_WORDBREAK;

	HMODULE user32 = LoadLibrary(_T("USER32.DLL"));
	_pAnimateWindow = (PANIMATEWINDOW)GetProcAddress(user32, "AnimateWindow");
	FreeLibrary(user32);

	SetDefaultFont();
}
SplashScreen::~SplashScreen()
{
    DeleteObject(_bitmap);
    DeleteObject(_customFont);
}

void SplashScreen::Create(HWND hwndParent, LPCTSTR text, DWORD timeout, DWORD style)
{
	if (hwndParent == NULL)
	    return;

	_hwndParent = hwndParent;
	_text = (text ? text : _T(""));
	_timeout = timeout;
	_style = style;

	WNDCLASSEX wcx = {0};
	wcx.cbSize = sizeof(wcx);
	wcx.lpfnWndProc = WindowProc;
	wcx.style = CS_DBLCLKS|CS_SAVEBITS;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 4;
	wcx.hInstance = GetModuleHandle(NULL);
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcx.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
	wcx.lpszClassName = _T("SplashScreenClass");

	if (_style & CSS_SHADOW)
		wcx.style |= CS_DROPSHADOW;

	//Didn't work? try not using dropshadow (may not be supported)
	if (!RegisterClassEx(&wcx))
	{
		if (_style & CSS_SHADOW)
		{
			wcx.style &= ~CS_DROPSHADOW;
			RegisterClassEx(&wcx);
		}
	}

	_splashScreen = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, _T("SplashScreenClass"),
        NULL, WS_POPUP, 0,0,0,0, hwndParent, NULL, GetModuleHandle(NULL), (LPVOID)this);
}
void SplashScreen::Show()
{
	SetWindowPos(_splashScreen, NULL, _xPos,_yPos, _bitmapWidth, _bitmapHeight,
        SWP_NOOWNERZORDER | SWP_NOZORDER);

	if (_style & CSS_FADEIN && _pAnimateWindow != NULL)
		_pAnimateWindow(_splashScreen, 500, AW_BLEND);
	else
		ShowWindow(_splashScreen, SW_SHOW);

	if (_timeout != 0)
		SetTimer(_splashScreen, 0, _timeout, NULL);
}
void SplashScreen::Hide()
{
	if (_style & CSS_FADEOUT && _pAnimateWindow != NULL)
		_pAnimateWindow(_splashScreen, 200, AW_HIDE | AW_BLEND);
	else
		ShowWindow(_splashScreen, SW_HIDE);

	DestroyWindow(_splashScreen);
}
void SplashScreen::Invalidate()
{
    RedrawWindow(_splashScreen, NULL, NULL,
        RDW_INTERNALPAINT|RDW_ERASE|RDW_INVALIDATE|RDW_ERASENOW|RDW_UPDATENOW);
}

HRGN SplashScreen::CreateRgnFromBitmap(HBITMAP bmp, COLORREF color)
{
    //This code is written by Davide Pizzolato

    if (!bmp)
        return NULL;

    BITMAP bm;
    GetObject(bmp, sizeof(BITMAP), &bm); // get bitmap attributes

    HDC hdc = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, bmp);

    std::vector<RECT> regions;

    // scan the image to find areas of solid regions to draw
    for (int y = 0; y < bm.bmHeight; ++y )
    {
        int nState = -1;
        int nStart = 0;
        int x = 0;
        for ( ; x < bm.bmWidth; ++x )
        {
            // get color
            bool isMask = (GetPixel(memDC, x, bm.bmHeight-y-1) == color);

            switch( nState )
            {
                case -1: // first pixel
                    if( isMask )
                        nState = 0;
                    else
                        nState = 1;
                    nStart = 0;
                    break;

                case 1: // solid
                    if( isMask )
                    {
                        // create region
                        RECT rc = { nStart, bm.bmHeight - y - 1, x, bm.bmHeight - y };
                        regions.push_back(rc);
                        nState = 0;
                    }
                    break;

                case 0: // transaparent
                    if( !isMask )
                    {
                        // start region
                        nStart = x;
                        nState = 1;
                    }
                    break;
            }
        }

        // solid last pixel?
        if( nStart != x-1 && nState==1)
        {
            // add region
            RECT rc = { nStart, bm.bmHeight - y - 1, bm.bmWidth, bm.bmHeight - y };
            regions.push_back(rc);
        }
    }

    SelectObject(memDC, oldBmp);
    DeleteDC(memDC);
    ReleaseDC(NULL, hdc);

    // create region
    // Under WinNT the ExtCreateRegion returns NULL (by Fable@aramszu.net)
    // HRGN hRgn = ExtCreateRegion( NULL, RDHDR + pRgnData->nCount * sizeof(RECT), (LPRGNDATA)pRgnData );
    // ExtCreateRegion replacement {
    HRGN hRgn = CreateRectRgn(0, 0, 0, 0);
    assert(hRgn);

    for(int i = 0; i<regions.size(); ++i)
    {
        RECT rc = regions[i];
        HRGN hr = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
        assert(CombineRgn(hRgn, hRgn, hr, RGN_OR) != ERROR);
        if (hr)
            DeleteObject(hr);
    }

    assert(hRgn);
    return hRgn;
}
void SplashScreen::DrawWindow(HDC hdc)
{
	//Blit Background
	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, _bitmap);
	BitBlt(hdc, 0,0, _bitmapWidth,_bitmapHeight, memDC, 0,0, SRCCOPY);
	SelectObject(memDC, oldBitmap);
	DeleteDC(memDC);

	//Draw Text
	SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, _textColor);

	HFONT oldFont = (HFONT)SelectObject(hdc, _customFont);
	DrawText(hdc, _text.c_str(), _text.size(), &_rcText, _textFormat);
	SelectObject(hdc, oldFont);
}

BOOL SplashScreen::OnEraseBkgnd(HWND hwnd, HDC pDC)
{
	return TRUE;
}
void SplashScreen::OnPaint(HWND hwnd)
{
 	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	DrawWindow(hdc);
	EndPaint(hwnd, &ps);
}
LRESULT SplashScreen::OnPrintClient(HWND hwnd, WPARAM wParam)
{
	HDC hdc = (HDC)wParam;
	DrawWindow(hdc);
	return 1;
}

LRESULT CALLBACK SplashScreen::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SplashScreen *ss = (SplashScreen*)GetWindowLong(hwnd, GWL_USERDATA);

    if (msg == WM_NCCREATE)
    {
        LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;
        ss = (SplashScreen*)cs->lpCreateParams;
        SetWindowLong(hwnd, GWL_USERDATA, (LONG)ss);
    }

    if (ss)
    {
        switch(msg)
        {
            HANDLE_MSG(hwnd, WM_ERASEBKGND, ss->OnEraseBkgnd);
            HANDLE_MSG(hwnd, WM_PAINT, ss->OnPaint);

            case WM_PRINTCLIENT:
                ss->OnPrintClient(hwnd, wParam);
                break;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
