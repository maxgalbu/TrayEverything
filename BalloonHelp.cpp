// *****************************************************************************
// BalloonHelp.cpp : implementation file
// Copyright 2001-2002, Joshua Heyer
//  You are free to use this code for whatever you want, provided you
// give credit where credit is due.  (I seem to get a lot of questions
// about that statement...  All i mean is, don't copy huge bits of code
// and then claim you wrote it.  You don't have to put my name in an about
// box or anything.  Though i'm not going to stop you if that's really what
// you want :~) )
//  I'm providing this code in the hope that it is useful to someone, as i have
// gotten much use out of other peoples code over the years.
//  If you see value in it, make some improvements, etc., i would appreciate it 
// if you sent me some feedback.
//
// ******************************************************************************

#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "BalloonHelp.h"

// allow multimonitor-aware code on Win95 systems
// comment out the first line if you have already define it in another file
// comment out both lines if you don't care about Win95
#define COMPILE_MULTIMON_STUBS


#ifndef DFCS_HOT
#define DFCS_HOT 0x1000
#endif

#ifndef AW_HIDE
#define AW_HIDE 0x00010000
#define AW_BLEND 0x00080000
#endif

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW   0x00020000
#endif

#ifndef SPI_GETDROPSHADOW
#define SPI_GETDROPSHADOW  0x1024
#endif

#ifndef SPI_GETTOOLTIPANIMATION
#define SPI_GETTOOLTIPANIMATION 0x1016
#endif

#ifndef SPI_GETTOOLTIPFADE
#define SPI_GETTOOLTIPFADE 0x1018
#endif

#ifndef SPI_GETMOUSEHOVERWIDTH
#define SPI_GETMOUSEHOVERWIDTH 98 
#endif

#ifndef MONITOR_DEFAULTTONEAREST
#define MONITOR_DEFAULTTONEAREST 2
#endif

#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED 0x031A 
#endif

// Kill timer
#define ID_TIMER_CLOSE  1

BalloonHelp::BalloonHelp() :
    _theme(NULL), _themeLib(0), _options(0), _timeout(0), _timerClose(0), _title(_T("")), _content(_T("")), 
	_hwndAnchor(NULL), _clickInfo(NULL), 
	_titleFont(NULL), _contentFont(NULL), 
	_foregroundColor( GetSysColor(COLOR_INFOTEXT) ), _backgroundColor( GetSysColor(COLOR_INFOBK) ), 
	_closeBtn_isPressed(0), _closeBtn_isClicked(0), _closeBtn_isHovered(0), 
	_classAtom(0), _classAtomShadowed(0),
	_pMonitorFromPoint(NULL), _pAnimateWindow(NULL), _pOpenThemeData(NULL), 
	_pDrawThemeBackground(NULL), _pCloseThemeData(NULL)
{
    _anchor.x = _anchor.y = 0;
	SetRectEmpty(&_screenRect);
	_rgnComplete = CreateRectRgn(0,0,0,0);
	
	_clickInfo = new MSG;
    memset(_clickInfo, 0, sizeof(MSG));
	
  	_user32 = LoadLibrary(_T("USER32.DLL"));
   	if (_user32)
   	{
   	  	_pAnimateWindow = (PFNANIMATEWINDOW)GetProcAddress(_user32, "AnimateWindow");
   	  	_pMonitorFromPoint = (PFNMMONITORFROMPOINT)GetProcAddress(_user32, "MonitorFromPoint");
	}
    
    _uxtheme = LoadLibrary(_T("UXTHEME.DLL"));
    if(_uxtheme)
    {
		_pOpenThemeData = (PFNOPENTHEMEDATA)GetProcAddress(_uxtheme, "OpenThemeData");
		_pDrawThemeBackground = (PFNDRAWTHEMEBACKGROUND)GetProcAddress(_uxtheme, "DrawThemeBackground");
		_pCloseThemeData = (PFNCLOSETHEMEDATA)GetProcAddress(_uxtheme, "CloseThemeData");
        
		if (_pOpenThemeData && _pDrawThemeBackground && _pCloseThemeData)
			_themeLib = TRUE;
		else
		{
			FreeLibrary(_uxtheme);
			_uxtheme = NULL;
		}
	}
}
BalloonHelp::~BalloonHelp()
{
    delete _clickInfo;
    
    DeleteObject(_contentFont);
    DeleteObject(_titleFont);
    ImageList_Destroy(_imageList);
    
    FreeLibrary(_uxtheme);
    FreeLibrary(_user32);
}

BOOL BalloonHelp::Create(LPCTSTR title, LPCTSTR content, POINT anchor,
    LPCTSTR iconStr, int options, HWND parent, MSG *clickInfo, int timeout)
{
    HINSTANCE inst = GetModuleHandle(NULL);
    
    _title = title;
    _content = content;
    _options = options;
    _timeout = timeout;
    
    if (clickInfo)
        *_clickInfo = *clickInfo;
    
    // if no fonts set, use defaults
    if (!_contentFont)
        _contentFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    // title font defaults to bold version of content font
    if (!_titleFont)
    {
        //_titleFont = new HFONT;
        LOGFONT lf;
        GetObject(_contentFont, sizeof(LOGFONT), &lf);
        lf.lfWeight = FW_BOLD;
        _titleFont = CreateFontIndirect(&lf);
    }
   
    ATOM wndClass = GetClassAtom(!(_options & BHO_DISABLE_XP_SHADOW));
    if (!wndClass)  // couldn't register class
        return FALSE;
    
    // check system settings: if fade effects are disabled or unavailable, disable here too
    BOOL bFade = FALSE;
    SystemParametersInfo(SPI_GETTOOLTIPANIMATION, 0, &bFade, 0);
    if (bFade)
        SystemParametersInfo(SPI_GETTOOLTIPFADE, 0, &bFade, 0);
    if (!bFade || NULL == _pAnimateWindow)
        _options |= BHO_DISABLE_FADE;
    
    
    // create invisible at arbitrary position; then position, set region, and finally show
    
    // the idea with WS_EX_TOOLWINDOW is, you can't switch to this using alt+tab
    DWORD exStyles = WS_EX_TOOLWINDOW;
    if (_options & BHO_SHOW_TOPMOST)
        exStyles |= WS_EX_TOPMOST;
    
    _balloon = CreateWindowEx(exStyles, reinterpret_cast<LPTSTR>(wndClass), _title.c_str(), 
        WS_POPUP, 0,0,10,10, parent, NULL, inst, this);
    if ( _balloon == NULL)
        return FALSE;
    
    if (!_theme && _themeLib)
        _theme = _pOpenThemeData(_balloon, L"Tooltip");
    
    SetAnchorPoint(anchor, parent);
    
    if (iconStr)
    {
        HICON icon = NULL;
        if (iconStr == IDI_APPLICATION || iconStr == IDI_INFORMATION
         || iconStr == IDI_ERROR || iconStr == IDI_QUESTION)
        {
            LoadIcon(NULL, iconStr);
        }
        else
        {
            icon = LoadIcon(inst, iconStr);
        }
        
        if (icon)
        {
            SetIconScaled(icon, 16, 16);
            DestroyIcon(icon);
        }
    }

    /*// these need to take effect even if the window receiving them
    // is not owned by this process.  So, if this process does not
    // already have the mouse captured, capture it!
    if (_options & BHO_CLOSE_ON_LBUTTON)
    {
        // no, i don't particularly need or want to deal with a situation
        // where a _balloon is being created and another program has captured
        // the mouse.  If you need it, it shouldn't be too hard, just do it here.
        if ( NULL == GetCapture() )
            SetCapture(_balloon);
    }*/
    
    
    Show();
    return TRUE;
}

void BalloonHelp::SetTitleFont(HFONT font)
{
    if ( NULL != _titleFont )
        DeleteObject(_titleFont);
    _titleFont = font;
    
    // if already visible, resize & move
    if (_balloon)
        PositionWindow();
}
void BalloonHelp::SetContentFont(HFONT font)
{
    if (_contentFont)
        DeleteObject(_contentFont);
    _contentFont = font;
    
    // if already visible, resize & move
    if (_balloon)
        PositionWindow();
}
void BalloonHelp::SetIcon(HICON icon)
{
    if (_imageList)
        ImageList_Destroy(_imageList);
    
    ICONINFO iconinfo;
    if (icon && GetIconInfo(icon, &iconinfo))
    {
        SetIcon(iconinfo.hbmColor, iconinfo.hbmMask);
        DeleteObject(iconinfo.hbmColor);
        DeleteObject(iconinfo.hbmMask);
    }
   
    // if already visible, resize & move (icon size may have changed)
    if (_balloon)
        PositionWindow();
}
void BalloonHelp::SetIconScaled(HICON icon, int cx, int cy)
{
    HDC hdc = GetDC(_balloon);
    HDC memDC = CreateCompatibleDC(hdc);
    
    HBITMAP tempBmp = CreateCompatibleBitmap(hdc, cx, cy);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, tempBmp);
    
    RECT rc = {0,0,cx,cy};
    HBRUSH brush = CreateSolidBrush(_backgroundColor);
    FillRect(memDC, &rc, brush);
    DeleteObject(brush);
    
    DrawIconEx(memDC, 0,0, icon, cx, cy, 0, NULL, DI_NORMAL);
    
    SelectObject(memDC, oldBmp);
    SetIcon(tempBmp, _backgroundColor);
    
    DeleteObject(tempBmp);
    DeleteDC(memDC);
    ReleaseDC(_balloon,hdc);
}
void BalloonHelp::SetIcon(HBITMAP bitmap, COLORREF crMask)
{
    if (_imageList)
        ImageList_Destroy(_imageList);
   
    if (bitmap)
    {
        BITMAP bm;
        GetObject(bitmap, sizeof(bm),(LPVOID)&bm);
        
        _imageList = ImageList_Create(bm.bmWidth, bm.bmHeight, ILC_COLOR32|ILC_MASK,1,0);
        ImageList_AddMasked(_imageList, bitmap, crMask);
    }
   
    // if already visible, resize & move (icon size may have changed)
    if (_balloon)
        PositionWindow();
}
void BalloonHelp::SetIcon(HBITMAP bBitmap, HBITMAP hMask)
{
    if (_imageList)
        ImageList_Destroy(_imageList);

    BITMAP bm;
    if (GetObject(bBitmap, sizeof(bm),(LPVOID)&bm))
    {
        _imageList = ImageList_Create(bm.bmWidth, bm.bmHeight, ILC_COLOR32|ILC_MASK,1,0);
        ImageList_Add(_imageList, bBitmap, hMask);
    }
    
    // if already visible, resize & move (icon size may have changed)
    if (_balloon)
        PositionWindow();
}
void BalloonHelp::SetIcon(HIMAGELIST imageList, int iconIndex)
{
    HICON icon = NULL;
    if (imageList && iconIndex >= 0 && iconIndex < ImageList_GetImageCount(imageList))
        icon = ImageList_ExtractIcon(NULL, imageList, iconIndex);
    
    SetIcon(icon);
    if (icon)
        DestroyIcon(icon);
    
    // if already visible, resize & move (icon size may have changed)
    if (_balloon)
        PositionWindow();
}
void BalloonHelp::SetClickInfo(MSG *clickInfo)
{
    *_clickInfo = *clickInfo;
}
void BalloonHelp::SetTimeout(int timeout)
{
    _timeout = timeout;
    // if timer is already set, reset.
    if (_balloon)
    {
        if ( _timeout > 0 )
            _timerClose = SetTimer(_balloon, ID_TIMER_CLOSE, _timeout, NULL);
        else
            KillTimer(_balloon, _timerClose);
    }
}
void BalloonHelp::SetAnchorPoint(POINT anchor, HWND hwndAnchor)
{
    _anchor = anchor;
    _hwndAnchor = hwndAnchor;
    
    // if already visible, move
    if (_balloon)
        PositionWindow();
}
void BalloonHelp::SetTitle(LPTSTR title)
{
    _title = title;
    
    // if already visible, resize & move
    if (_balloon)
        PositionWindow();
}
void BalloonHelp::SetContent(LPTSTR content)
{
    _content = content;
    
    // if already visible, resize & move
    if (_balloon)
        PositionWindow();
}
void BalloonHelp::SetForegroundColor(COLORREF crForeground)
{
    _foregroundColor = crForeground;
    
    // repaint if visible
    if (_balloon)
        InvalidateRect(_balloon, NULL, FALSE);
}
void BalloonHelp::SetBackgroundColor(COLORREF crBackground)
{
    _backgroundColor = crBackground;
    
    // repaint if visible
    if (_balloon)
        InvalidateRect(_balloon, NULL, FALSE);
}

void BalloonHelp::DrawNonClientArea(HDC hdc)
{
    RECT rect;
    GetWindowRect(_balloon, &rect);
    MapWindowPoints(NULL, _balloon, (LPPOINT)&rect, 2);
    
    RECT rectClient;
    GetClientRect(_balloon, &rectClient);
    OffsetRect(&rectClient, -rect.left, -rect.top);
    OffsetRect(&rect, -rect.left, -rect.top);
    ExcludeClipRect(hdc, rectClient.left, rectClient.top, rectClient.right, rectClient.bottom);
    
    HBRUSH bgBrush = CreateSolidBrush(_backgroundColor);
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    SelectClipRgn(hdc, NULL);
    
    //ASSERT(NULL != m_rgnComplete.m_hObject);
    HBRUSH brushFg = CreateSolidBrush(_foregroundColor);
    if ( _options & BHO_SHOW_INNERSHADOW )
    {
        HBRUSH brushHL;
        
        // slightly lighter color
        int red = 170 + GetRValue(_backgroundColor)/3;
        int green = 170 + GetGValue(_backgroundColor)/3;
        int blue = 170 + GetBValue(_backgroundColor)/3;
        brushHL = CreateSolidBrush(RGB(red,green,blue));
        OffsetRgn(_rgnComplete, 1,1);
        FrameRgn(hdc, _rgnComplete, brushHL, 2, 2);
        
        DeleteObject(brushHL);
        
        // slightly darker color
        red = GetRValue(_foregroundColor)/3 + GetRValue(_backgroundColor)/3*2;
        green = GetGValue(_foregroundColor)/3 + GetGValue(_backgroundColor)/3*2;
        blue = GetBValue(_foregroundColor)/3 + GetBValue(_backgroundColor)/3*2;
        OffsetRgn(_rgnComplete, -2,-2);
        brushHL = CreateSolidBrush(RGB(red,green,blue));
        FrameRgn(hdc, _rgnComplete, brushHL, 2, 2);
        OffsetRgn(_rgnComplete, 1,1);
        
        DeleteObject(brushHL);
    }
    
    // outline
    FrameRgn(hdc, _rgnComplete, brushFg, 1, 1);
    DeleteObject(brushFg);
}
void BalloonHelp::DrawClientArea(HDC hdc)
{
   SIZE sizeHeader = DrawHeader(hdc, TRUE);
   DrawContent(hdc, sizeHeader.cy+TIP_MARGIN, TRUE);
}
SIZE BalloonHelp::DrawHeader(HDC hdc, BOOL draw)
{     
    SIZE headerSize = {0,0};
    RECT rectClient;
    GetClientRect(_balloon, &rectClient);   // use this for positioning when drawing
    // else if content is wider than title, centering wouldn't work

    // calc & draw icon
    if (_imageList)
    {
        int x = 0;
        int y = 0;
        ImageList_GetIconSize(_imageList, &x, &y);
        
        headerSize.cx += x;
        headerSize.cy = std::max((int)headerSize.cy, y);
        ImageList_SetBkColor(_imageList, _backgroundColor);
        
        if (draw)
            ImageList_Draw(_imageList, 0, hdc, 0, 0, ILD_NORMAL);
        
        rectClient.left += x;
    }

    // calc & draw close button
    if (_options & BHO_SHOW_CLOSEBUTTON )
    {
        int btnWidth = GetSystemMetrics(SM_CXSMSIZE);
        // if something is already in the header (icon) leave space
        if ( headerSize.cx > 0 )
            headerSize.cx += TIP_MARGIN;
        
        headerSize.cx += btnWidth;
        headerSize.cy = std::max((int)headerSize.cy, GetSystemMetrics(SM_CYSMSIZE));
        if (draw)
        {
            RECT rc = {rectClient.right-btnWidth, 0, rectClient.right, GetSystemMetrics(SM_CYSMSIZE)};
            if (_theme)
                _pDrawThemeBackground(_theme, hdc, TTP_CLOSE, TTCS_NORMAL, &rc, NULL);
            else
                DrawFrameControl(hdc, &rc, DFC_CAPTION, DFCS_CAPTIONCLOSE|DFCS_FLAT);
        }
        rectClient.right -= btnWidth;
    }
   
    // calc title size
    if (_title.size())
    {
        HFONT oldFont = (HFONT)SelectObject(hdc, _titleFont);
        
        // if something is already in the header (icon or close button) leave space
        if ( headerSize.cx > 0 ) 
            headerSize.cx += TIP_MARGIN;
        RECT rectTitle = {0,0,0,0};
        DrawText(hdc, _title.c_str(), _title.size(), &rectTitle, DT_CALCRECT|DT_NOPREFIX|
            DT_EXPANDTABS|DT_SINGLELINE);
        
        headerSize.cx += rectTitle.right - rectTitle.left;
        headerSize.cy = std::max(headerSize.cy, rectTitle.bottom - rectTitle.top);
        
        // draw title
        if (draw)
        {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, _foregroundColor);
            
            rectClient.left += 6;
            DrawText(hdc, _title.c_str(), _title.size(), &rectClient, DT_LEFT|DT_NOPREFIX|
                DT_EXPANDTABS|DT_SINGLELINE);
        }
        
        // cleanup
        SelectObject(hdc, oldFont);
    }
   
    return headerSize;
}
SIZE BalloonHelp::DrawContent(HDC hdc, int nTop, BOOL draw)
{   
    RECT rectContent;
    GetAnchorScreenBounds(rectContent);
    
    OffsetRect(&rectContent, -rectContent.left, -rectContent.top);
    rectContent.top = nTop;
    
    // limit to half screen width
    rectContent.right -= (rectContent.right - rectContent.left)/2;
    
    // calc size
    HFONT oldFont = (HFONT)SelectObject(hdc, _contentFont);
    if (_content.size())
        DrawText(hdc, _content.c_str(), _content.size(), &rectContent, DT_CALCRECT | DT_LEFT | 
            DT_NOPREFIX | DT_EXPANDTABS | DT_WORDBREAK);
    else
        SetRectEmpty(&rectContent);   // don't want to leave half the screen for empty strings ;)
    
    // draw
    if (draw == TRUE)
    {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, _foregroundColor);
        DrawText(hdc, _content.c_str(), _content.size(), &rectContent, DT_LEFT | DT_NOPREFIX | DT_WORDBREAK | DT_EXPANDTABS);
    }
    
    // cleanup
    SelectObject(hdc, oldFont);
    
    SIZE sz = { rectContent.right - rectContent.left, rectContent.bottom - rectContent.top};
    return sz;
}

SIZE BalloonHelp::CalcHeaderSize(HDC pDC)
{
    return DrawHeader(pDC, FALSE);
}
SIZE BalloonHelp::CalcContentSize(HDC pDC)
{
    return DrawContent(pDC, 0, FALSE);
}
void BalloonHelp::GetAnchorScreenBounds(RECT& rect)
{
    if (IsRectEmpty(&_screenRect))
    {     
        if (_pMonitorFromPoint)
        {           
            // get the nearest monitor to the anchor
            HMONITOR monitor = _pMonitorFromPoint(_anchor, MONITOR_DEFAULTTONEAREST);
            
            // get the monitor bounds
            MONITORINFO mi = {0};
            mi.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(monitor, &mi);
            
            // work area (area not obscured by task bar, etc.)
            _screenRect = mi.rcWork;
        }
    }
    
    rect = _screenRect;
}
BALLOON_QUADRANT BalloonHelp::GetBalloonQuadrant()
{
    RECT rectDesktop;
    GetAnchorScreenBounds(rectDesktop);
   
    if ( _anchor.y < rectDesktop.top + (rectDesktop.bottom - rectDesktop.top)/2 )
    {
        if ( _anchor.x < rectDesktop.left + (rectDesktop.right - rectDesktop.left)/2 )
            return BQ_TOPLEFT;
        else
            return BQ_TOPRIGHT;
    }
    else
    {
        if ( _anchor.x < rectDesktop.left + (rectDesktop.right - rectDesktop.left)/2 )
            return BQ_BOTTOMLEFT;
        else
            return BQ_BOTTOMRIGHT;
    }

   // unreachable
}
SIZE BalloonHelp::CalcClientSize()
{
    SIZE sz = {0};
    if (_balloon)
    {
        HDC hdc = GetWindowDC(_balloon);
        SIZE sizeHeader = CalcHeaderSize(hdc);
        SIZE sizeContent = CalcContentSize(hdc);
        ReleaseDC(_balloon, hdc);
        
        sz.cx = std::max(sizeHeader.cx,sizeContent.cx);
        sz.cy = sizeHeader.cy + TIP_MARGIN + sizeContent.cy;
    }
    
    return sz;
}
SIZE BalloonHelp::CalcWindowSize()
{
    SIZE size = CalcClientSize();
    size.cx += TIP_MARGIN*2;
    size.cy += TIP_TAIL+TIP_MARGIN*2;
    //size.cx = max(size.cx, nTIP_MARGIN*2+nTIP_TAIL*4);
    return size;
}
void BalloonHelp::PositionWindow()
{
    SIZE windowSize = CalcWindowSize();
    
    POINT ptTail[3];
    POINT ptTopLeft = {0,0};
    POINT ptBottomRight = {windowSize.cx, windowSize.cy};
    
    // force recalculation of desktop rect
    SetRectEmpty(&_screenRect);
    
    switch (GetBalloonQuadrant())
    {
        case BQ_TOPLEFT:
            ptTopLeft.y = TIP_TAIL;
            ptTail[0].x = (windowSize.cx-TIP_TAIL)/4 + TIP_TAIL;
            ptTail[0].y = TIP_TAIL+1;
            ptTail[2].x = (windowSize.cx-TIP_TAIL)/4;
            ptTail[2].y = ptTail[0].y;
            ptTail[1].x = ptTail[2].x;
            ptTail[1].y = 1;
            break;
        case BQ_TOPRIGHT:
            ptTopLeft.y = TIP_TAIL;
            ptTail[0].x = (windowSize.cx-TIP_TAIL)/4*3;
            ptTail[0].y = TIP_TAIL+1;
            ptTail[2].x = (windowSize.cx-TIP_TAIL)/4*3 + TIP_TAIL;
            ptTail[2].y = ptTail[0].y;
            ptTail[1].x = ptTail[2].x;
            ptTail[1].y = 1;
            break;
        case BQ_BOTTOMLEFT:
            ptBottomRight.y = windowSize.cy-TIP_TAIL;
            ptTail[0].x = (windowSize.cx-TIP_TAIL)/4 + TIP_TAIL;
            ptTail[0].y = windowSize.cy-TIP_TAIL-2;
            ptTail[2].x = (windowSize.cx-TIP_TAIL)/4;
            ptTail[2].y = ptTail[0].y;
            ptTail[1].x = ptTail[2].x;
            ptTail[1].y = windowSize.cy-2;
            break;
        case BQ_BOTTOMRIGHT:
            ptBottomRight.y = windowSize.cy-TIP_TAIL;
            ptTail[0].x = (windowSize.cx-TIP_TAIL)/4*3;
            ptTail[0].y = windowSize.cy-TIP_TAIL-2;
            ptTail[2].x = (windowSize.cx-TIP_TAIL)/4*3 + TIP_TAIL;
            ptTail[2].y = ptTail[0].y;
            ptTail[1].x = ptTail[2].x;
            ptTail[1].y = windowSize.cy-2;
            break;
    }
    
    // adjust for very narrow balloons
    if ( ptTail[0].x < TIP_MARGIN )
        ptTail[0].x = TIP_MARGIN;
    if ( ptTail[0].x > windowSize.cx - TIP_MARGIN )
        ptTail[0].x = windowSize.cx - TIP_MARGIN;
    if ( ptTail[1].x < TIP_MARGIN )
        ptTail[1].x = TIP_MARGIN;
    if ( ptTail[1].x > windowSize.cx - TIP_MARGIN )
        ptTail[1].x = windowSize.cx - TIP_MARGIN;
    if ( ptTail[2].x < TIP_MARGIN )
        ptTail[2].x = TIP_MARGIN;
    if ( ptTail[2].x > windowSize.cx - TIP_MARGIN )
        ptTail[2].x = windowSize.cx - TIP_MARGIN;
    
    // get window position
    POINT ptOffs = {_anchor.x - ptTail[1].x, _anchor.y - ptTail[1].y};
    
    // adjust position so all is visible
    RECT rectScreen;
    GetAnchorScreenBounds(rectScreen);
    
    int adjustX = 0;
    int adjustY = 0;
    if ( ptOffs.x < rectScreen.left )
        adjustX = rectScreen.left-ptOffs.x;
    else if ( ptOffs.x + windowSize.cx >= rectScreen.right )
        adjustX = rectScreen.right - (ptOffs.x + windowSize.cx);
    if ( ptOffs.y + TIP_TAIL < rectScreen.top )
        adjustY = rectScreen.top - (ptOffs.y + TIP_TAIL);
    else if ( ptOffs.y + windowSize.cy - TIP_TAIL >= rectScreen.bottom )
        adjustY = rectScreen.bottom - (ptOffs.y + windowSize.cy - TIP_TAIL);
    
    // reposition tail
    // uncomment two commented lines below to move entire tail 
    // instead of just anchor point
    
    //ptTail[0].x -= nAdjustX;
    ptTail[1].x -= adjustX;
    //ptTail[2].x -= nAdjustX;
    ptOffs.x += adjustX;
    ptOffs.y += adjustY;
    
    // place window
    MoveWindow(_balloon, ptOffs.x, ptOffs.y, windowSize.cx, windowSize.cy, TRUE);
    
    // apply region
    HRGN region;
    HRGN regionRound;
    HRGN regionComplete;
    region = CreatePolygonRgn(&ptTail[0], 3, ALTERNATE);
    regionRound = CreateRoundRectRgn(ptTopLeft.x,ptTopLeft.y,ptBottomRight.x,ptBottomRight.y,TIP_MARGIN*3,TIP_MARGIN*3);
    regionComplete = CreateRectRgn(0,0,1,1);
    CombineRgn(regionComplete, region, regionRound, RGN_OR);

    if (!_rgnComplete)
        _rgnComplete = CreateRectRgn(0,0,1,1);

    if ( !EqualRgn(_rgnComplete, regionComplete) )
    {
        CombineRgn(_rgnComplete, regionComplete, NULL, RGN_COPY);
        SetWindowRgn(_balloon, (HRGN)regionComplete, TRUE);
        
        // There is a bug with layered windows and NC changes in Win2k
        // As a workaround, redraw the entire window if the NC area changed.
        // Changing the anchor point is the ONLY thing that will change the
        // position of the client area relative to the window during normal
        // operation.
        PostMessage(_balloon, WM_PRINT, (WPARAM)GetDC(_balloon), PRF_CLIENT);
        UpdateWindow(_balloon);
    }
}
ATOM BalloonHelp::GetClassAtom(BOOL shadowed)
{
    if ( 0 == _classAtom )
    {
        WNDCLASSEX wcx = {0};
        wcx.cbSize = sizeof(wcx);
        wcx.style = CS_DBLCLKS|CS_SAVEBITS|CS_DROPSHADOW;
        wcx.lpfnWndProc = BalloonProc;
        wcx.cbClsExtra = 0;
        wcx.cbWndExtra = 4;
        wcx.hInstance = GetModuleHandle(NULL); 
        wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcx.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
        wcx.lpszClassName = _T("BalloonHelpClassDS");
        
        // Register the window class (this may not work if dropshadows are not supported)
        _classAtomShadowed = RegisterClassEx(&wcx);
        
        // Register shadow-less class
        wcx.style &= ~CS_DROPSHADOW;
        wcx.lpszClassName = _T("BalloonHelpClass");
        _classAtom = RegisterClassEx(&wcx);
    }
   
    if ( shadowed && 0 != _classAtomShadowed )
        return _classAtomShadowed;
    return _classAtom;
}

void BalloonHelp::Show()
{
    ShowWindow(_balloon, SW_SHOWNOACTIVATE);
    if ( !(_options & BHO_DELAY_CLOSE) )
        SetTimeout(_timeout);     // start close timer
}
void BalloonHelp::Hide()
{
    if ( _options & BHO_DELAY_CLOSE )
    {
        _options &= ~(BHO_DELAY_CLOSE|BHO_CLOSE_ON_ANYTHING);  // close only via timer or button
        SetTimeout(_timeout);     // start close timer
        return;
    }
    
    ShowWindow(_balloon, SW_HIDE);
    if ( GetCapture() == _balloon ) 
        ReleaseCapture();
    
    DestroyWindow(_balloon);
    DeleteObject(_rgnComplete);
}

LRESULT CALLBACK BalloonHelp::BalloonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BalloonHelp *bh = (BalloonHelp*)GetWindowLong(hwnd, GWL_USERDATA);
    
    if (msg == WM_NCCREATE)
    {
        LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;
        bh = (BalloonHelp*)cs->lpCreateParams;
        SetWindowLong(hwnd, GWL_USERDATA, (LONG)bh);
    }
    
    switch(msg)
    {
        case WM_ERASEBKGND:     return bh->OnEraseBkgnd((HDC)wParam);
        case WM_PRINTCLIENT:    return bh->OnPrintClient(wParam, lParam);
        case WM_PRINT:          bh->OnPrint(wParam, lParam); break;
        case WM_NCPAINT:        bh->OnNcPaint((HRGN)wParam); break;
        case WM_PAINT:          bh->OnPaint(); break;
        
        case WM_NCCALCSIZE:     bh->OnNcCalcSize(wParam, (NCCALCSIZE_PARAMS FAR*)lParam); break;
        case WM_NCHITTEST:      return bh->OnNcHitTest(LOWORD(lParam), HIWORD(lParam));
        
        case WM_SHOWWINDOW:     bh->OnShowWindow(wParam, lParam); break;
        case WM_TIMER:          bh->OnTimer(wParam); break;
        case WM_THEMECHANGED:   bh->OnThemeChanged(); break;
        case WM_CLOSE:          bh->OnClose(); break;
        case WM_DESTROY:        bh->OnDestroy(); break;
        
        case WM_KEYUP:          bh->OnKeyUp(wParam, lParam);
        case WM_LBUTTONDOWN:    bh->OnLButtonDown(wParam, LOWORD(lParam), HIWORD(lParam)); break;
        case WM_LBUTTONUP:      bh->OnLButtonUp(wParam, LOWORD(lParam), HIWORD(lParam)); break;
        case WM_MOUSEMOVE:      bh->OnMouseMove(wParam, LOWORD(lParam), HIWORD(lParam)); break;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT BalloonHelp::OnEraseBkgnd(HDC hdc)
{
   RECT rect;
   GetClientRect(_balloon, &rect);
   
   HBRUSH bgBrush = CreateSolidBrush(_backgroundColor);
   FillRect(hdc, &rect, bgBrush);
   DeleteObject(bgBrush);
   
   return TRUE;
}
LRESULT BalloonHelp::OnPrintClient(WPARAM wParam, LPARAM lParam)
{
    HDC hdc = (HDC)wParam;
    if (lParam & PRF_ERASEBKGND)
        SendMessage(_balloon, WM_ERASEBKGND, wParam, 0);
    if (lParam & PRF_CLIENT)
        DrawClientArea(hdc);
    
    return 0;
}
void BalloonHelp::OnPrint(WPARAM wParam, LPARAM lParam)
{
    HDC hdc = (HDC)wParam;
    if (lParam & PRF_NONCLIENT) 
        DrawNonClientArea(hdc);
}
void BalloonHelp::OnPaint()
{
    LPPAINTSTRUCT ps;
    HDC hdc = BeginPaint(_balloon, ps);
    DrawClientArea(hdc);
    EndPaint(_balloon, ps);
}
void BalloonHelp::OnNcPaint(HRGN hrgn)
{
    HDC hdc = GetWindowDC(_balloon);
    DrawNonClientArea(hdc);
    ReleaseDC(_balloon, hdc);
}
LRESULT BalloonHelp::OnNcHitTest(int x, int y)
{
   return HTCLIENT;
}
void BalloonHelp::OnNcCalcSize(BOOL fCalcValidRects, NCCALCSIZE_PARAMS * lpncsp)
{
    // TIP_MARGIN pixel margin on all sides
    InflateRect(&lpncsp->rgrc[0], -TIP_MARGIN, -TIP_MARGIN);

    // TIP_TAIL pixel "tail" on side closest to anchor
    switch ( GetBalloonQuadrant() )
    {
        case BQ_TOPRIGHT:
        case BQ_TOPLEFT:
            lpncsp->rgrc[0].top += TIP_TAIL;
            break;
        case BQ_BOTTOMRIGHT:
        case BQ_BOTTOMLEFT:
            lpncsp->rgrc[0].bottom -= TIP_TAIL;
            break;
    }
    
    // sanity: ensure rect does not have negative size
    if ( lpncsp->rgrc[0].right < lpncsp->rgrc[0].left )
        lpncsp->rgrc[0].right = lpncsp->rgrc[0].left;
    if ( lpncsp->rgrc[0].bottom < lpncsp->rgrc[0].top )
        lpncsp->rgrc[0].bottom = lpncsp->rgrc[0].top;

    if ( fCalcValidRects )
    {
        // determine if client position has changed relative to the window position
        // if so, don't bother presearving anything.
        if ( !EqualRect(&lpncsp->rgrc[0], &lpncsp->rgrc[2]) )
            SetRectEmpty(&lpncsp->rgrc[2]);
    }
}
void BalloonHelp::OnKeyUp(int vKey, int keyData)
{
    if (_options & BHO_CLOSE_ON_KEYPRESS)
        Hide();
}
void BalloonHelp::OnLButtonDown(UINT keyFlags, int x, int y)
{
    POINT point = {x, y};
    if (_options & BHO_CLOSE_ON_LBUTTON)
        Hide();
    else if (_options & BHO_SHOW_CLOSEBUTTON)
    {
        RECT rect;
        GetClientRect(_balloon, &rect);
        rect.left = rect.right-GetSystemMetrics(SM_CXSIZE);
        rect.bottom = rect.top+GetSystemMetrics(SM_CYSIZE);
        if ( PtInRect(&rect, point) )
        {
            _closeBtn_isPressed = TRUE;
            _closeBtn_isClicked = TRUE;
            
            SetCapture(_balloon);
            OnMouseMove(keyFlags, x, y);
        }
    }
}
void BalloonHelp::OnLButtonUp(UINT keyFlags, int x, int y)
{
    POINT point = {x, y};
    if (_options & BHO_SHOW_CLOSEBUTTON && _closeBtn_isPressed)
    {
        _closeBtn_isPressed = FALSE;
        _closeBtn_isClicked = FALSE;
        
        RECT rect;
        GetClientRect(_balloon, &rect);
        rect.left = rect.right- GetSystemMetrics(SM_CXSMSIZE);
        rect.bottom = rect.top+  GetSystemMetrics(SM_CYSMSIZE);
        
        if ( PtInRect(&rect, point) )
            Hide();
    }
    else if (_clickInfo != NULL)
    {
        RECT rect;
        GetClientRect(_balloon, &rect);
        if ( PtInRect(&rect, point) )
        {
            SendMessage(_clickInfo->hwnd, _clickInfo->message, _clickInfo->wParam, _clickInfo->lParam);
            Hide();
        }
    }
}
void BalloonHelp::OnMouseMove(UINT keyFlags, int x, int y)
{
    POINT point = {x, y};
    if (_options & BHO_CLOSE_ON_MOUSEMOVE)
        Hide();
    else if (_options & BHO_SHOW_CLOSEBUTTON)
    {
        RECT rect;
        GetClientRect(_balloon, &rect);
        rect.left = rect.right - GetSystemMetrics(SM_CXSMSIZE);
        rect.bottom = rect.top + GetSystemMetrics(SM_CYSMSIZE);
        
        if (PtInRect(&rect, point))
        {
            _closeBtn_isHovered = TRUE;
            
            if (_closeBtn_isClicked)
                _closeBtn_isPressed = TRUE;
        }
        else
        {
            _closeBtn_isHovered = FALSE;
            
            if (_closeBtn_isClicked)
                _closeBtn_isPressed = FALSE;
        }
        
        HDC hdc = GetDC(_balloon);
        if (_theme)
        {
            int state = 0;
            if (_closeBtn_isPressed)
                state = TTCS_PRESSED;
            else if (_closeBtn_isHovered)
                state = TTCS_HOT;
            else
                state = TTCS_NORMAL;
            
            _pDrawThemeBackground(_theme, hdc, TTP_CLOSE, state, &rect, NULL);
        }
        else
        {
            int state = 0;
            if (_closeBtn_isPressed)
                state = DFCS_PUSHED;
            else if (_closeBtn_isHovered)
                state = DFCS_HOT;
            else
                state = DFCS_FLAT;
            
            DrawFrameControl(hdc, &rect, DFC_CAPTION, DFCS_CAPTIONCLOSE|state);
        }
        
        ReleaseDC(_balloon, hdc);
    }
}
void BalloonHelp::OnShowWindow(BOOL show, UINT nStatus)
{
    if (_pAnimateWindow)
    {
        if ( show && !(_options & BHO_DISABLE_FADEIN) )
            _pAnimateWindow(_balloon, 200, AW_BLEND);
        else if ( !show && !(_options & BHO_DISABLE_FADEOUT) )
            _pAnimateWindow(_balloon, 200, AW_HIDE | AW_BLEND );
    }
}
void BalloonHelp::OnTimer(UINT id)
{
    // really shouldn't be any other timers firing, but might as well make sure
    if (id == ID_TIMER_CLOSE)
    {
        KillTimer(_balloon, _timerClose);
        Hide();
    }
}
void BalloonHelp::OnDestroy()
{
    delete this;
}
void BalloonHelp::OnClose()
{
    Hide();
}
void BalloonHelp::OnThemeChanged()
{
    if (_theme) _pCloseThemeData(_theme);
    _theme = _pOpenThemeData(_balloon, L"Tooltip");
}
