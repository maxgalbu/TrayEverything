// ******************************************************************************
// BalloonHelp.cpp : header file
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
// ?/??/02 - Release #3: incomplete
//
//    Minor changes, bug fixes.
// I fixed an ugly bug where deallocated memory was accessed when using the
// strURL parameter to open a file or location when the balloon was clicked.
// I also fixed a minor but very visible bug in the demo.
// Added a couple of new options: 
//    unDELAY_CLOSE works in tandem with a timeout value to delay the action
// caused by the other unCLOSE_* options.  This allows you to keep a balloon
// active indefinately, until the user gets back from coffee break, etc., and
// has time to take a look at it.  For long timeout values, i'd advise also
// using unSHOW_CLOSE_BUTTON so the user can get rid of it quickly if need be.
//    unDISABLE_XP_SHADOW is exactly what it sounds like: if set, that cool
// dropshadow XP uses for tooltips and menus isn't shown.  Note that the user
// can also disable dropshadows globally, in which case this option has no effect.
// ---
// 5/30/02 - Release #2: i begin versioning
//
//    I suppose i should have kept a progress log for this.
// But i didn't, so you'll just have to assume it was mostly right to begin with.
// And i'll further confuse this by versioning it R2, even though it's the 
// fourth release (?) (i haven't been keeping track, heheh).
// I will, however, thank several people who have shown me better ways to do
// things, and thus improved the class greatly.
//
// Thanks to:
// Jan van den Baard for showing me the right way to use AnimateWindow(),
//    and for demonstrating how WM_NCHITTEST can be used to provide hot tracking.
//    Check out his ClassLib library on CP!
// Maximilian Hänel for his WTL port, and for demonstrating therein a
//    nicer way to handle message hooks.
// Mustafa Demirhan for the original idea and code for using keyboard hooks.
// All the other people who have provided suggestions, feeback, and code on the
// CP forums.  This class would not be half as useful as it is without you all.
//
// ******************************************************************************


#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <tmschema.h>
#include <string>

#define TIP_TAIL                        20
#define TIP_MARGIN                      8

#define BHO_CLOSE_ON_LBUTTON            1
#define BHO_CLOSE_ON_MOUSEMOVE          2
#define BHO_CLOSE_ON_KEYPRESS           4
#define BHO_CLOSE_ON_ANYTHING           BHO_CLOSE_ON_LBUTTON|BHO_CLOSE_ON_MOUSEMOVE|BHO_CLOSE_ON_KEYPRESS
#define BHO_DELAY_CLOSE                 8
#define BHO_SHOW_CLOSEBUTTON            16
#define BHO_SHOW_INNERSHADOW            32
#define BHO_SHOW_TOPMOST                64
#define BHO_DISABLE_XP_SHADOW           128
#define BHO_DISABLE_FADEIN              256
#define BHO_DISABLE_FADEOUT             512
#define BHO_DISABLE_FADE                BHO_DISABLE_FADEIN|BHO_DISABLE_FADEOUT

enum BALLOON_QUADRANT { BQ_TOPRIGHT, BQ_TOPLEFT, BQ_BOTTOMRIGHT, BQ_BOTTOMLEFT };

class BalloonHelp
{
    public:
        BalloonHelp();
        ~BalloonHelp();
        
        BOOL Create(LPCTSTR title,          // title of balloon
            LPCTSTR content,                // content of balloon
            POINT anchor,                   // anchor (tail position) of balloon
            LPCTSTR iconStr,                // icon to display
            int options,                    // options (see above)
            HWND parent,                    // parent window (NULL == MFC main window)
            MSG *clickInfo,                 // Message info (hwnd, msg, wParam, lParam) to send when clicked
            int timeout);                   // delay before closing automatically (milliseconds)       
        
        // Sets the font used for drawing the balloon title.  Deleted by balloon, do not use CFont* after passing to this function.
        void SetTitleFont(HFONT font);
        // Sets the font used for drawing the balloon content.  Deleted by balloon, do not use CFont* after passing to this function.
        void SetContentFont(HFONT font);
        // Sets the icon displayed in the top left of the balloon (pass NULL to hide icon)
        void SetIcon(HICON icon);
        // Sets the icon displayed in the top left of the balloon (pass NULL to hide icon)
        void SetIconScaled(HICON icon, int cx, int cy);
        // Sets the icon displayed in the top left of the balloon (pass NULL hBitmap to hide icon)
        void SetIcon(HBITMAP bitmap, COLORREF crMask);
        // Sets the icon displayed in the top left of the balloon
        void SetIcon(HBITMAP bitmap, HBITMAP hMask);
        // Sets icon displayed in the top left of the balloon to image # nIconIndex from pImageList
        void SetIcon(HIMAGELIST imageList, int nIconIndex);
        // Sets the message to be sent when balloon is clicked. Pass NULL to disable.
        void SetClickInfo(MSG *clickInfo);
        // Sets the number of milliseconds the balloon can remain open.  Set to 0 to disable timeout.
        void SetTimeout(int timeout);
        // Sets the point to which the balloon is "anchored"
        void SetAnchorPoint(POINT anchor, HWND hwndAnchor = NULL);
        // Sets the title of the balloon
        void SetTitle(LPTSTR title);
        // Sets the content of the balloon (plain text only)
        void SetContent(LPTSTR content);
        // Sets the forground (text and border) color of the balloon
        void SetForegroundColor(COLORREF foregroundColor);
        // Sets the background color of the balloon
        void SetBackgroundColor(COLORREF backgrounColor);
    
    private:
        typedef BOOL (WINAPI* PFNANIMATEWINDOW)(HWND,DWORD,DWORD);
        typedef HMONITOR (WINAPI* PFNMMONITORFROMPOINT)(POINT,DWORD);
        typedef HRESULT(WINAPI *PFNCLOSETHEMEDATA)(HTHEME);
        typedef HRESULT(WINAPI *PFNDRAWTHEMEBACKGROUND)(HTHEME,HDC,int,int, const LPRECT, const LPRECT);
        typedef HTHEME(WINAPI *PFNOPENTHEMEDATA)(HWND,LPCWSTR);
        typedef std::basic_string<TCHAR> tstring;
        
        HWND _balloon;
        HMODULE _user32;
        HMODULE _uxtheme;
        HTHEME _theme;
        BOOL _themeLib;
        
        int _options;
        int _timeout;
        int _timerClose;
        tstring _title;
        tstring _content;
        POINT _anchor;
        HWND _hwndAnchor;
        MSG *_clickInfo;
        RECT _screenRect;
        HFONT _titleFont;
        HFONT _contentFont;
        COLORREF _foregroundColor;
        COLORREF _backgroundColor;
        HIMAGELIST _imageList; 
        HRGN _rgnComplete;
        
        BOOL _closeBtn_isPressed;
        BOOL _closeBtn_isClicked;
        BOOL _closeBtn_isHovered;
        
        ATOM _classAtom;
        ATOM _classAtomShadowed;
        
        PFNMMONITORFROMPOINT _pMonitorFromPoint;
        PFNANIMATEWINDOW _pAnimateWindow;
        PFNOPENTHEMEDATA _pOpenThemeData;
        PFNDRAWTHEMEBACKGROUND _pDrawThemeBackground;
        PFNCLOSETHEMEDATA _pCloseThemeData;
        
        static LRESULT CALLBACK BalloonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        
        //Message handlers
        LRESULT OnPrintClient(WPARAM wParam, LPARAM lParam);
        LRESULT OnEraseBkgnd(HDC pDC);
        LRESULT OnNcHitTest(int x, int y);
        void OnPrint(WPARAM wParam, LPARAM lParam);
        void OnPaint();
        void OnNcPaint(HRGN hrgn);
        void OnNcCalcSize(BOOL fCalcValidRects, NCCALCSIZE_PARAMS *lpncsp);
        void OnKeyUp(int vKey, int keyData);
        void OnLButtonDown(UINT keyFlags, int x, int y);
        void OnLButtonUp(UINT keyFlags, int x, int y);
        void OnMouseMove(UINT keyFlags, int x, int y);
        void OnShowWindow(BOOL show, UINT status);
        void OnTimer(UINT id);
        void OnClose();
        void OnDestroy();
        void OnThemeChanged();
        
        SIZE CalcHeaderSize(HDC pDC);
        SIZE CalcContentSize(HDC pDC);
        SIZE CalcClientSize();
        SIZE CalcWindowSize();
        void GetAnchorScreenBounds(RECT& rect);
        BALLOON_QUADRANT GetBalloonQuadrant();
        void PositionWindow();
        ATOM GetClassAtom(BOOL bShadowed);
        
        void DrawNonClientArea(HDC pDC);
        void DrawClientArea(HDC pDC);
        SIZE DrawHeader(HDC pDC, BOOL bDraw);
        SIZE DrawContent(HDC pDC, int nTop, BOOL bDraw);
        
        void Show();
        void Hide();
};
