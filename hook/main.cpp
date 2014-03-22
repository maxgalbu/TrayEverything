#include "../resource.h"

#include <shlwapi.h>
#include <uxtheme.h>
#include <tmschema.h>

#define CEXPORT                     extern "C" _declspec(dllexport) 
#define SUBCLASSPROP                _T("SubclassProp")
#define BTNEDGE                     4

typedef struct tagSUBCLASSSTRUCT
{
    tagSUBCLASSSTRUCT() : 
        originalProc(NULL), 
        minimizeOption(0),
            haltRedraw(0), hovered(0), clicked(0), pressed(0),
            minButtonChoose(FALSE), minButtonOption(FALSE)
    {
        SetRectEmpty(&buttonClientRect);
        SetRectEmpty(&buttonScreenRect);
    }
    
    WNDPROC originalProc;
    UINT minimizeOption;
    
    //Tray button variables
    RECT buttonClientRect; //Rect where the button is drawn
    RECT buttonScreenRect; //Rect to handle mouse move and mouse click
    BOOL haltRedraw;
    BOOL hovered;
    BOOL clicked;
    BOOL pressed;
    
    //Minimize button variables
    BOOL minButtonChoose;
    BOOL minButtonOption;
} SUBCLASSSTRUCT;

UINT hookedMsg = 0;
UINT unhookedMsg = 0;
UINT minimizeMsg = 0;
//UINT externMinimizeMsg = 0; //dont't fit here
//UINT taskbarCreatedMsg = 0; //dont't fit here
UINT getHookOptionsMsg = 0;
UINT setHookOptionsMsg = 0;
UINT updateHookOptionsMsg = 0;
BOOL subclass = TRUE;

BOOL IsCommCtrlVersion6()
{
    static BOOL isCommCtrlVersion6 = -1;
    if (isCommCtrlVersion6 != -1)
        return isCommCtrlVersion6;
    
    //The default value
    isCommCtrlVersion6 = FALSE;
    
    HINSTANCE commCtrlDll = LoadLibrary(_T("comctl32.dll"));
    if (commCtrlDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(commCtrlDll, "DllGetVersion");
        
        if (pDllGetVersion)
        {
            DLLVERSIONINFO dvi = {0};
            dvi.cbSize = sizeof(DLLVERSIONINFO);
            (*pDllGetVersion)(&dvi);
            
            isCommCtrlVersion6 = (dvi.dwMajorVersion == 6);
        }
        
        FreeLibrary(commCtrlDll);
    }
    
    return isCommCtrlVersion6;
}

HMODULE Themes_GetDll(BOOL destroy = FALSE)
{
    static HMODULE themeDll = NULL;
    
    if (destroy)
    {
        FreeLibrary(themeDll);
        themeDll = NULL;
    }
    if (themeDll != NULL)
        return themeDll;
    
    themeDll = LoadLibrary(_T("uxtheme.dll"));
    return themeDll;
}
HTHEME Themes_Get(HWND hwnd, BOOL destroy = FALSE)
{
    static HTHEME theme = NULL;
    if (theme != NULL)
        return theme;
    
    HMODULE themeDll = Themes_GetDll();
    
    if (destroy)
    {
        typedef HRESULT (WINAPI *PFNCLOSETHEMEDATA)(HTHEME);
        PFNCLOSETHEMEDATA pCloseThemeData = (PFNCLOSETHEMEDATA)GetProcAddress(themeDll, "CloseThemeData");
        pCloseThemeData(theme);
        theme = NULL;
    }
    else
    {
        typedef HTHEME (WINAPI *PFNOPENTHEMEDATA)(HWND,LPCWSTR);
        PFNOPENTHEMEDATA pOpenThemeData = (PFNOPENTHEMEDATA)GetProcAddress(themeDll, "OpenThemeData");
        theme = pOpenThemeData(hwnd, L"Window");
    }
    
    return theme;
}
void Themes_Destroy(HWND hwnd)
{
    Themes_Get(hwnd, TRUE);
    Themes_GetDll(TRUE);
}
void Themes_DrawBackground(HWND hwnd, HDC hdc, int partId, int stateId, LPRECT rect)
{
    typedef HRESULT (WINAPI *PFNDRAWTHEMEBACKGROUND)(HTHEME,HDC,int,int,const LPRECT,const LPRECT);
    
    static PFNDRAWTHEMEBACKGROUND pDrawThemeBackground = NULL;
    if (pDrawThemeBackground == NULL)
    {
        HMODULE themeDll = Themes_GetDll();
        pDrawThemeBackground = (PFNDRAWTHEMEBACKGROUND)GetProcAddress(themeDll, "DrawThemeBackground");
    }
    
    HTHEME theme = Themes_Get(hwnd);
    pDrawThemeBackground(theme, hdc, partId, stateId, rect, NULL);
}

int Edge_CalcRight(HWND hwnd)
{
	DWORD style = GetWindowLong(hwnd, GWL_STYLE);

	if(style & WS_THICKFRAME)
		return GetSystemMetrics(SM_CXSIZEFRAME);
	else
		return GetSystemMetrics(SM_CXFIXEDFRAME);

}
int Edge_GetRightOffset(HWND hwnd)
{
	DWORD styles = GetWindowLong(hwnd, GWL_STYLE);
	DWORD exStyles = GetWindowLong(hwnd, GWL_EXSTYLE);
    
	int btnSize = 0;
	if(exStyles & WS_EX_TOOLWINDOW)
	{
		int sysBtnSize = GetSystemMetrics(SM_CXSMSIZE) - BTNEDGE;
        
		if(styles & WS_SYSMENU)	
			btnSize += sysBtnSize + BTNEDGE;
        
		return btnSize + Edge_CalcRight(hwnd);
	}
	else
	{
		int sysBtnSize = GetSystemMetrics(SM_CXSIZE) - BTNEDGE;
        
		//Window has Close [X] button. This button has a 2-pixel
		//border on either size
		if(styles & WS_SYSMENU)
			btnSize += sysBtnSize + BTNEDGE;
        
		//If either of the minimize or maximize buttons are shown,
		//Then both will appear (but may be disabled)
		//This button pair has a 2 pixel border on the left
		if(styles & (WS_MINIMIZEBOX | WS_MAXIMIZEBOX) )
			btnSize += BTNEDGE + sysBtnSize * 2;
        
		//A window can have a question-mark button, but only
		//if it doesn't have any min/max buttons
		else if(exStyles & WS_EX_CONTEXTHELP)
			btnSize += BTNEDGE + sysBtnSize;
		
		//Now calculate the size of the border...aggghh!
		return btnSize + Edge_CalcRight(hwnd);
	}
}

void RedrawNonClientArea(HWND hwnd)
{
    RedrawWindow(hwnd, NULL, NULL, RDW_FRAME|RDW_INVALIDATE|RDW_UPDATENOW);
}
HWND GetTrayEverythingWindow()
{
    return FindWindow(_T("TrayEverythingDialog"), NULL);
}
void Reset(HWND hwnd)
{
    SUBCLASSSTRUCT *ss = (SUBCLASSSTRUCT*)GetProp(hwnd, SUBCLASSPROP);
    ss->minimizeOption = 0;
    
    //Reset system menu
    GetSystemMenu(hwnd, TRUE);
}

LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SUBCLASSSTRUCT *ss = (SUBCLASSSTRUCT*)GetProp(hwnd, SUBCLASSPROP);
    if (!ss)
        return 0;
    
    switch(msg)
    {
        default:
        {
            if (msg == hookedMsg || msg == updateHookOptionsMsg)
            {
                HWND teWnd = GetTrayEverythingWindow();
                SendMessage(teWnd, getHookOptionsMsg, (WPARAM)hwnd, 0);
            }
            else if (msg == setHookOptionsMsg)
            {
                switch(wParam)
                {
                    case TRAY_HOOK:
                    {
                        Reset(hwnd);
                        
                        ss->minimizeOption = lParam;
                        if (ss->minimizeOption == HO_TRAYBUTTON)
                        {
                            SendMessage(hwnd, WM_SIZE, 0,0);
                            RedrawNonClientArea(hwnd);
                        }
                        else if (ss->minimizeOption == HO_MINBUTTON)
                        {
                            /*HMENU systemMenu = GetSystemMenu(hwnd, FALSE);
                            {
                                UINT state = MFS_GRAYED|MFS_CHECKED;
                                if (ss->minButtonChoose)
                                {
                                    state = 0;
                                    if (ss->minButtonOption)
                                        state |= MFS_CHECKED;
                                }
                                
                                MENUITEMINFO mii = {0};
                                
                                ZeroMemory(&mii, sizeof(MENUITEMINFO));
                                mii.cbSize = sizeof(MENUITEMINFO);
                                mii.fMask = MIIM_ID|MIIM_STRING|MIIM_STATE;
                                mii.fState = state;
                                mii.wID = IDMC_LS_MENU_MINTOTRAY;
                                mii.dwTypeData = _T("Put to tray when minimized");
                                InsertMenuItem(systemMenu, SC_CLOSE, FALSE, &mii);
                                
                                ZeroMemory(&mii, sizeof(MENUITEMINFO));
                                mii.cbSize = sizeof(MENUITEMINFO);
                                mii.fMask = MIIM_FTYPE;
                                mii.fType = MFT_SEPARATOR;
                                InsertMenuItem(systemMenu, SC_CLOSE, FALSE, &mii);
                            }*/
                        }
                    }
                    break;
                }
            }
            else if (msg == unhookedMsg || msg == WM_NCDESTROY)
            {
                CallWindowProc(ss->originalProc, hwnd, msg, wParam, lParam);
                
                //Avoid ulterior subclass
                subclass = FALSE;
                
                //Reset the original state
                Reset(hwnd);
                
                RemoveProp(hwnd, SUBCLASSPROP);
                SetWindowLong(hwnd, GWL_WNDPROC, (LONG)ss->originalProc);
                delete ss;
                
                return 0;
            }
        }
        break;
        
        case WM_ACTIVATE:
        case WM_MOVE:
        case WM_SIZE:
        case WM_NCHITTEST:
        case WM_NCLBUTTONDOWN:
        case WM_NCMOUSEMOVE:
        case WM_NCLBUTTONUP:
        case WM_NCPAINT:
        {
            //If this window doesn't have a titlebar, don't handle the message
            //If the option isn't set, don't do anything
            if (GetWindowLong(hwnd, GWL_STYLE) & WS_CAPTION && ss->minimizeOption == HO_TRAYBUTTON)
            {
                switch(msg)
                {
                    case WM_ACTIVATE:
                        RedrawNonClientArea(hwnd);
                        break;
                    
                    case WM_MOVE:
                    case WM_SIZE:
                    {
                        int rightOffset = Edge_GetRightOffset(hwnd);
                        RECT windowRect; GetWindowRect(hwnd, &windowRect);
                        
                        int client_lastButtonPos = (windowRect.right-windowRect.left) - rightOffset;
                        int screen_lastButtonPos = windowRect.right - rightOffset;
                        
                        ss->buttonClientRect.left = client_lastButtonPos-24;
                        ss->buttonClientRect.top = 6;
                        ss->buttonClientRect.right = client_lastButtonPos-2;
                        ss->buttonClientRect.bottom = 6+22;
                        
                        ss->buttonScreenRect.left = screen_lastButtonPos-24;
                        ss->buttonScreenRect.top = windowRect.top+6;
                        ss->buttonScreenRect.right = screen_lastButtonPos-2;
                        ss->buttonScreenRect.bottom = windowRect.top+6+22;
                        
                        RedrawNonClientArea(hwnd);
                    }
                    break;
                    
                    case WM_NCHITTEST:
                    {
                        POINT pt; GetCursorPos(&pt);
                        if (PtInRect(&ss->buttonScreenRect, pt))
                            return HTBORDER;
                    }
                    break;
                    
                    case WM_NCLBUTTONDOWN:
                    {
                        POINT pt; GetCursorPos(&pt);
                        if (PtInRect(&ss->buttonScreenRect, pt))
                        {
                            ss->clicked = TRUE;
                            ss->pressed = TRUE;
                        }
                        
                        RedrawNonClientArea(hwnd);
                    }
                    break;
                    
                    case WM_NCLBUTTONUP:
                    {
                        POINT pt; GetCursorPos(&pt);
                        if (PtInRect(&ss->buttonScreenRect, pt))
                        {
                            ss->clicked = FALSE;
                            ss->pressed = FALSE;
                            
                            //Send a minimize message to TrayEverything
                            HWND teWnd = GetTrayEverythingWindow();
                            SendMessage(teWnd, minimizeMsg, (WPARAM)hwnd, 0);
                        }
                        else
                        {
                            if (ss->clicked)
                            {
                                ss->pressed = FALSE;
                                ss->clicked = FALSE;
                            }
                        }
                        
                        RedrawNonClientArea(hwnd);
                    }
                    break;
                    
                    case WM_NCMOUSEMOVE:
                    {
                        POINT pt; GetCursorPos(&pt);
                        if (ss->clicked)
                        {
                            BOOL old = ss->pressed;
                            
                            if (!PtInRect(&ss->buttonScreenRect, pt))
                                ss->pressed = FALSE;
                            else
                                ss->pressed = TRUE;
                            
                            if (old != ss->pressed)
                                RedrawNonClientArea(hwnd);
                        }
                        else
                        {
                            BOOL old = ss->hovered;
                            
                            if (PtInRect(&ss->buttonScreenRect, pt))
                                ss->hovered = TRUE;
                            else
                                ss->hovered = FALSE;
                            
                            if (old != ss->hovered)
                                RedrawNonClientArea(hwnd);
                        }
                    }
                    break;
                    
                    case WM_NCPAINT:
                    {
                        if (ss->haltRedraw == TRUE)
                            break;
                        
                        CallWindowProc(ss->originalProc, hwnd, msg, wParam, lParam);
                        HDC hdc = GetWindowDC(hwnd);
                        
                        if (IsCommCtrlVersion6())
                        {
                            int state = 0;
                            if (ss->pressed)
                                state = MINBS_PUSHED;
                            else if (ss->hovered)
                                state = MINBS_HOT;
                            else
                                state = MINBS_NORMAL;
                            
                            Themes_DrawBackground(hwnd, hdc, WP_MINBUTTON, state, &ss->buttonClientRect);
                        }
                        else
                        {
                            int state = 0;
                            if (ss->pressed)
                                state = DFCS_PUSHED;
                            
                            DrawFrameControl(hdc, &ss->buttonClientRect, DFC_CAPTION, DFCS_CAPTIONMIN|state);
                        }
                        
                        ReleaseDC(hwnd, hdc);
                    }
                    return 0;
                }
            }
        }
        break;
        
        //Hack to refresh the system menu immediately
        //(it never refresh on the first click, unless it's on the program icon)
        case WM_NCRBUTTONDOWN:
        {
            HWND teWnd = GetTrayEverythingWindow();
            SendMessage(teWnd, getHookOptionsMsg, (WPARAM)hwnd, 0);
        }
        break;
        
        //Middle button click
        case WM_NCMBUTTONDOWN:
        {
            if (ss->minimizeOption == HO_MIDDLECLICK)
            {
                HWND teWnd = GetTrayEverythingWindow();
                SendMessage(teWnd, minimizeMsg, (WPARAM)hwnd, 0);
            }
        }
        break;
        
        case WM_INITMENUPOPUP:
        {
            HMENU systemMenu = GetSystemMenu(hwnd, FALSE);
            HMENU menu = (HMENU)wParam;
            
            //It's the window menu?
            if (menu == systemMenu)
            {
                HWND teWnd = GetTrayEverythingWindow();
                SendMessage(teWnd, getHookOptionsMsg, (WPARAM)hwnd, 0);
                return 0;
            }
        }
        break;
        
        case WM_SYSCOMMAND:
        {
            if (ss->minimizeOption == HO_MINBUTTON)
            {
                if (wParam == SC_MINIMIZE)
                {
                    HWND teWnd = GetTrayEverythingWindow();
                    SendMessage(teWnd, minimizeMsg, (WPARAM)hwnd, 0);
                    
                    //SetWindowLong(hwnd, DWL_MSGRESULT, TRUE);
                    return TRUE;
                }
                else if (wParam == IDMC_LS_MENU_MINTOTRAY)
                {
                    HMENU systemMenu = GetSystemMenu(hwnd, FALSE);
                    
                    //Check whether the menu item is checked or not
                    MENUITEMINFO mii = {0};
                    mii.cbSize = sizeof(MENUITEMINFO);
                    mii.fMask = MIIM_STATE;
                    GetMenuItemInfo(systemMenu, IDMC_LS_MENU_MINTOTRAY, FALSE, &mii);
                    
                    if (mii.fState & MFS_CHECKED)
                        CheckMenuItem(systemMenu, IDMC_LS_MENU_MINTOTRAY, MF_BYCOMMAND|MF_UNCHECKED);
                    else
                        CheckMenuItem(systemMenu, IDMC_LS_MENU_MINTOTRAY, MF_BYCOMMAND|MF_CHECKED);
                    
                    //Update the "minimize" list of programs
                    HWND teWnd = GetTrayEverythingWindow();
                    SendMessage(teWnd, setHookOptionsMsg, PROGRAMOPTION_NOHOOKMINBUTTON, (LPARAM)hwnd);
                }
            }
        }
        break;
    }
    
    return CallWindowProc(ss->originalProc, hwnd, msg, wParam, lParam);
}
CEXPORT LRESULT CALLBACK GetMessageProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION && subclass)
    {
        if (hookedMsg == 0) hookedMsg = RegisterWindowMessage(WM_TE_HOOKED);
        if (unhookedMsg == 0) unhookedMsg = RegisterWindowMessage(WM_TE_UNHOOKED);
        if (minimizeMsg == 0) minimizeMsg = RegisterWindowMessage(WM_TE_MINIMIZE);
        if (getHookOptionsMsg == 0) getHookOptionsMsg = RegisterWindowMessage(WM_TE_GETHOOKOPTIONS);
        if (setHookOptionsMsg == 0) setHookOptionsMsg = RegisterWindowMessage(WM_TE_SETHOOKOPTIONS);
        if (updateHookOptionsMsg == 0) updateHookOptionsMsg = RegisterWindowMessage(WM_TE_UPDATEHOOKOPTIONS);
        
        MSG *msg = (MSG*)lParam;
        
        //If it's a top window, it's visible and enabled
        if (GetParent(msg->hwnd) == NULL && IsWindowVisible(msg->hwnd) && IsWindowEnabled(msg->hwnd))
        {
            //If this is not TrayEverything
            if (GetProp(msg->hwnd, _T("TrayWindows")) != NULL)
                subclass = FALSE;
            else
            {
                //If this window isn't already subclassed
                if (GetProp(msg->hwnd, SUBCLASSPROP) == NULL)
                {
                    SUBCLASSSTRUCT *ss = new SUBCLASSSTRUCT;
                    SetProp(msg->hwnd, SUBCLASSPROP, (HANDLE)ss);
                    
                    ss->originalProc = (WNDPROC)SetWindowLong(msg->hwnd, GWL_WNDPROC, (LONG)SubclassProc);
                    
                    Sleep(1000);
                    PostMessage(msg->hwnd, hookedMsg, 0,0);
                }
            }
        }
    }
    
    return CallNextHookEx(NULL, code, wParam, lParam);
}

BOOL WINAPI DllMain(HANDLE instance, DWORD reason, LPVOID reserved)
{
    switch(reason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
            break;
        
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    
    return TRUE;
}
