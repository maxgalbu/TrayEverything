#include "resource.h"

#include "BalloonHelp.h"
#include "SplashScreenEx.h"

#include "AboutDlg.h"
#include "CommonFuncs.h"
#include "HookFuncs.h"
#include "LangFuncs.h"
#include "MainFuncs.h"
#include "OptionsDlg.h"
#include "OptionsFuncs.h"
#include "StringConversions.h"
#include "UpdateDlg.h"
#include "WindowsList.h"

UINT externMinimizeMsg = 0;
UINT taskbarCreatedMsg = 0;
UINT getHookOptionsMsg = 0;
UINT setHookOptionsMsg = 0;
UINT updateHookOptionsMsg = 0;

BOOL isCompositionEnabled = FALSE;
DWMISCOMPOSITIONENABLED pDwmIsCompositionEnabled = NULL;
DWMEXTENDFRAMEINTOCLIENTAREA pDwmExtendFrameIntoClientArea = NULL;

LRESULT CALLBACK RebarSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void OpenWindows_Check(HWND hwnd)
{
    WindowsVector *windows = Windows_Get(hwnd);
    for (int i = 0; i<windows->size(); i++)
    {
        BOOL isWindow = IsWindow(windows->at(i).hwnd);
        BOOL isVisible = IsWindowVisible(windows->at(i).hwnd);

        if (!isWindow || (!isVisible && !windows->at(i).isMinimized))
        {
            windows->at(i).toRemove = TRUE;
            continue;
        }

        TCHAR windowText[256] = _T("");
        GetWindowText(windows->at(i).hwnd, windowText, 256);
        if (windows->at(i).windowText != windowText)
        {
            windows->at(i).toUpdate |= WTU_WINDOWTEXT;
            windows->at(i).windowText = windowText;
        }
    }
}
BOOL CALLBACK OpenWindows_EnumProc(HWND hwnd, LPARAM param)
{
    // Check if it's a window
    if (!IsWindow(hwnd) || !IsWindowVisible(hwnd))
        return TRUE;
    //Check if it's a top window
    if (GetParent(hwnd) != NULL)
        return TRUE;

    HWND hwndMain = (HWND)param;

    //Get window text
    TCHAR windowText[MAX_PATH] = _T("");
    GetWindowText(hwnd, windowText, MAX_PATH);

    if (tstring(windowText).empty())
        return TRUE;

    //Get the executable path and the process
    //id from the window handle
    tstring path = _T("");
    DWORD processID = 0;
    {
        //First, get the process id of the hwnd
        GetWindowThreadProcessId(hwnd, &processID);

        //Unser window XP, the PROCESSENTRY32 structure doesn't set the full
        //path of the executable of the process. But the MODULEENTRY32 structure
        //of the first module in the process has the full path to it (stored in
        //szExePath).
        if (IsWindowsXp())
        {
            // Take a snapshot of the modules of this process
            HANDLE moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);
            
            //Get the first module
            MODULEENTRY32 me = {0};
            me.dwSize = sizeof(MODULEENTRY32);
            Module32First(moduleSnapshot, &me);
            
            //Copy the path
            path = me.szExePath;
            
            CloseHandle(moduleSnapshot);
        }
        
        //Under windows 9x/ME, the PROCESSENTRY32 has already the full path to
        //the executable (stored in szExeFile). I only need to find the right process.
        if (!IsWindowsXp() || path.empty())
        {
            // Take a snapshot of all the processes
            HANDLE processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

            //Find the process that has the same id of the window
            PROCESSENTRY32 pe = {0};
            pe.dwSize = sizeof(PROCESSENTRY32);

            if (Process32First(processSnapshot, &pe))
            {
                do {
                    if (pe.th32ProcessID == processID)
                    {
                        //Copy the path
                        path = pe.szExeFile;
                        break;
                    }
                } while(Process32Next(processSnapshot, &pe));
            }

            CloseHandle(processSnapshot);
        }
    }
    
    if (DirName(path) == path)
    {
        TCHAR pathVariable[1024] = _T("");
        GetEnvironmentVariable(_T("PATH"), pathVariable, 1024);
        
        StringVector paths = explode(pathVariable, _T(";"));
        for (int i = 0; i < paths.size(); i++)
        {
            tstring testpath = paths[i] + _T("\\") + path;
            if (FileExists(testpath))
            {
                path = testpath;
                break;
            }
        }
    }
    
    //Get the filename
    tstring filename = FileName(path);

    std::transform(filename.begin(), filename.end(),
        filename.begin(), tolower);

    //We don't want to minimize ourselves, neither the desktop.
    if (processID != GetCurrentProcessId()
     && tstring(windowText) != _T("Program Manager"))
    {
        int index = FindWindow_FromHandle(hwndMain, hwnd);

        //Insert
        if (index == -1)
        {
            WINDOW window;
            window.hwnd = hwnd;
            window.processID = processID;
            window.path = path;
            window.filename = filename;
            window.windowText = windowText;

            WindowsVector *windows = Windows_Get(hwndMain);
            windows->push_back(window);
        }
    }

    return TRUE;
}

void Toolbar_GetSize(HWND hwndToolbar, SIZE *sz)
{
    //TB_GETMAXSIZE doesn't work correctly under
    //all Windows' versions, so i'm providing a 
    //manual implementation.
    
    sz->cx = 0;
    sz->cy = 0;
    
	int btnCount = ToolBar_ButtonCount(hwndToolbar);
	for(int i = 0; i<btnCount; i++)
	{
		RECT itemRect;
		ToolBar_GetItemRect(hwndToolbar, i, &itemRect);
		
		itemRect.right -= itemRect.left;
		sz->cx += itemRect.right;
		
		itemRect.bottom -= itemRect.top;
		if(itemRect.bottom > sz->cy )
			sz->cy = itemRect.bottom;
	}
}
void Rebar_Init(HWND rebar)
{
    REBARINFO ri;
    ri.cbSize = sizeof(REBARINFO);
    ri.fMask = 0;
    ri.himl = (HIMAGELIST)NULL;
    Rebar_SetBarInfo(rebar, &ri);
    
    WNDPROC originalProc = (WNDPROC)SetWindowLong(rebar, GWL_WNDPROC, (LONG)RebarSubclassProc);
    SetProp(rebar, ORIGINALPROCPROP, (HANDLE)originalProc);
}
void Rebar_AddBand(HWND hwndRebar, HWND hwndChild, UINT bandID, LPCTSTR bandHeader, BOOL isToolbar)
{
    REBARBANDINFO addRBI = {0};
    addRBI.cbSize = sizeof(REBARBANDINFO);
    addRBI.fMask  = RBBIM_LPARAM|RBBIM_IDEALSIZE|RBBIM_ID|RBBIM_STYLE|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE;
    addRBI.fStyle = RBBS_CHILDEDGE|RBBS_GRIPPERALWAYS|RBBS_USECHEVRON;
    addRBI.hwndChild  = hwndChild;
    addRBI.wID = bandID;
    addRBI.lParam = isToolbar;
    
    if (bandHeader != NULL)
    {
        addRBI.fMask |= RBBIM_TEXT;
        addRBI.lpText = (TCHAR*)bandHeader;
    }
    
    if (isToolbar)
    {
        SIZE sz;
        Toolbar_GetSize(hwndChild, &sz);
        
        addRBI.cxIdeal     = sz.cx;
        addRBI.cyMinChild  = sz.cy;
        addRBI.cx          = sz.cx+15; //15 = size of the grip
        addRBI.cxMinChild  = sz.cx;
    }
    else
    {
        RECT rc;
        GetWindowRect(hwndChild, &rc);
        
        addRBI.cxIdeal     = rc.right-rc.left;
        addRBI.cxMinChild  = rc.right-rc.left;
        addRBI.cyMinChild  = rc.bottom-rc.top;
        addRBI.cx          = rc.right-rc.left;
    }
    
    Rebar_InsertBand(hwndRebar, -1, &addRBI);
    
    if (bandHeader != NULL)
    {
        //Last band
        int bandIndex = Rebar_GetBandCount(hwndRebar) - 1;
        
        REBARBANDINFO modifyRBI = {0};
        modifyRBI.cbSize = sizeof(REBARBANDINFO);
        modifyRBI.fMask = RBBIM_HEADERSIZE|RBBIM_SIZE;
        Rebar_GetBandInfo(hwndRebar, bandIndex, &modifyRBI);
        
        modifyRBI.fMask = RBBIM_SIZE;
        modifyRBI.cx += modifyRBI.cxHeader-10;
        Rebar_SetBandInfo(hwndRebar, bandIndex, &modifyRBI);
    }
}

void MainToolbar_Init(HWND hwndMain)
{
    HWND rebar = GetDlgItem(hwndMain, IDC_REBAR);
    
    HWND toolBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD|WS_VISIBLE|
        WS_TABSTOP|WS_BORDER|TBSTYLE_BUTTON|TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|CCS_NODIVIDER, 0,0,0,0,
        rebar, (HMENU)IDC_TOOLBAR, GetModuleHandle(NULL), NULL);
    
    HFONT defaultFont = NULL;
    if (!isCompositionEnabled)
        defaultFont = GetStockFont(DEFAULT_GUI_FONT);
    else
    {
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, (PVOID)&ncm, 0);
        
        defaultFont = CreateFontIndirect(&ncm.lfCaptionFont);
        SetProp(toolBar, DEFAULTFONTPROP, (HANDLE)defaultFont);
    }
    SendMessage(toolBar, WM_SETFONT, (WPARAM)defaultFont, TRUE);
    
    ToolBar_ButtonStructSize(toolBar, sizeof(TBBUTTON));

    HIMAGELIST imageList = ImageList_Create(48,48, ILC_COLOR32|ILC_MASK, 6, 6);
    ToolBar_SetImageList(toolBar, imageList);

    HINSTANCE inst = GetModuleHandle(NULL);
    HBITMAP trayBmp = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_TRAY), IMAGE_BITMAP, 48,48, 0);
    HBITMAP settingBmp = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_SETTINGS), IMAGE_BITMAP, 48,48, 0);
    HBITMAP miminizeBmp = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_MINIMIZE), IMAGE_BITMAP, 48,48, 0);
    HBITMAP updateBmp = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_UPDATE), IMAGE_BITMAP, 48,48, 0);
    HBITMAP topBmp = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_TOPMOST), IMAGE_BITMAP, 48,48, 0);
    HBITMAP aboutBmp = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_ABOUT), IMAGE_BITMAP, 48,48, 0);
    HBITMAP exitBmp = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_EXIT), IMAGE_BITMAP, 48,48, 0);

    ImageList_AddMasked(imageList, trayBmp, RGB(255,255,255));          DeleteObject(trayBmp);
    ImageList_AddMasked(imageList, settingBmp, RGB(255,255,255));       DeleteObject(settingBmp);
    ImageList_AddMasked(imageList, miminizeBmp, RGB(255,255,255));      DeleteObject(miminizeBmp);
    ImageList_AddMasked(imageList, updateBmp, RGB(255,255,255));        DeleteObject(updateBmp);
    ImageList_AddMasked(imageList, topBmp, RGB(255,255,255));           DeleteObject(topBmp);
    ImageList_AddMasked(imageList, aboutBmp, RGB(255,255,255));         DeleteObject(aboutBmp);
    ImageList_AddMasked(imageList, exitBmp, RGB(255,255,255));          DeleteObject(exitBmp);
    
    tstring trayBtn = Lang_LoadString(toolBar, IDS_TB_TRAY);
    tstring settingsBtn = Lang_LoadString(toolBar, IDS_TB_SETTINGS);
    tstring minimizeBtn = Lang_LoadString(toolBar, IDS_TB_MINIMIZE);
    tstring updateBtn = Lang_LoadString(toolBar, IDS_TB_UPDATE);
    tstring topmostBtn = Lang_LoadString(toolBar, IDS_TB_TOPMOST);
    tstring aboutBtn = Lang_LoadString(toolBar, IDS_TB_ABOUT);
    tstring exitBtn = Lang_LoadString(toolBar, IDS_TB_EXIT);
    
    TBBUTTON tbb[] = {
        { 0, IDC_TB_TRAY, TBSTATE_ENABLED|TBSTATE_CHECKED, TBSTYLE_BUTTON , {0,0}, 0, (INT_PTR)trayBtn.c_str() },
        { 1, IDC_TB_SETTINGS, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, (INT_PTR)settingsBtn.c_str() },
        { 2, IDC_TB_MINIMIZE, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, (INT_PTR)minimizeBtn.c_str() },
        { 3, IDC_TB_UPDATE, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, (INT_PTR)updateBtn.c_str() },
        { 4, IDC_TB_TOPMOST, TBSTATE_ENABLED, TBSTYLE_CHECK, {0,0}, 0, (INT_PTR)topmostBtn.c_str() },
        { 5, IDC_TB_ABOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, (INT_PTR)aboutBtn.c_str() },
        { 6, IDC_TB_EXIT, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, (INT_PTR)exitBtn.c_str() },
    };
    
    ToolBar_AddButtons(toolBar, 7, &tbb);
    
    ToolBar_SetButtonSize(toolBar, 64, 66);
    ToolBar_SetBitmapSize(toolBar, 48, 48);
    ToolBar_AutoSize(toolBar);
    
    Rebar_AddBand(rebar, toolBar, 4000, _T("rebarname"), TRUE);
    SetLayeredWindowAttributes(toolBar, RGB(200,200,200), 0, LWA_COLORKEY);
}
void MainToolbar_Free(HWND hwndMain)
{
    HWND rebar = GetDlgItem(hwndMain, IDC_REBAR);
    HWND toolbar = GetDlgItem(rebar, IDC_TOOLBAR);
    
    HFONT defaultFont = (HFONT)GetProp(toolbar, DEFAULTFONTPROP);
    DeleteObject(defaultFont);
}
void StatusBar_Init(HWND statusBar)
{
    int statusWidths[] = {300, 400, -1};
    SendMessage(statusBar, SB_SETPARTS, 3, (LPARAM)statusWidths);
    SendMessage(statusBar, SB_SETTEXT, 0,
        (LPARAM)Lang_LoadString(statusBar, IDS_SB_READY).c_str());
}

void MainDlg_MinimizeRestoreAnimation(HWND hwnd, BOOL restore)
{
    RECT firstRect, secondRect;

    //Window rect
    GetWindowRect(hwnd, &firstRect);

    //Tray rect
    HWND tray = GetTrayWindow();
    GetWindowRect(tray, &secondRect);

    if (restore)
        DrawAnimatedRects(hwnd, IDANI_CAPTION, &secondRect, &firstRect);
    else
        DrawAnimatedRects(hwnd, IDANI_CAPTION, &firstRect, &secondRect);
}
void MainDlg_Restore(HWND hwnd)
{
    HWND hwndMain = GetMainWindow(hwnd);

    if (!IsWindowVisible(hwndMain))
    {
        if (Options_GetInt(hwndMain, MAINTRAY_HIDEONMIN))
            UnregisterHotKey(hwndMain, TEHOTKEY_ID);
        else
        {
            NOTIFYICONDATA nid = {0};
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwndMain;
            nid.uID = MAINTRAY_ID;
            Shell_NotifyIcon(NIM_DELETE, &nid);

            HMENU trayMenu = (HMENU)RemoveProp(hwndMain, MAINTRAYMENUPROP);
            DestroyMenu(trayMenu);
        }

        MainDlg_MinimizeRestoreAnimation(hwnd, TRUE);
        ShowWindow(hwndMain, SW_SHOW);
    }
}
void MainDlg_Minimize(HWND hwnd, HICON mainIcon)
{
    HWND hwndMain = GetMainWindow(hwnd);

    if (Options_GetInt(hwnd, MAINTRAY_HIDEONMIN))
        RegisterHotKey(hwndMain, TEHOTKEY_ID, MOD_WIN, 'T');
    else
    {
        NOTIFYICONDATA nid = {0};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwndMain;
        nid.uID = MAINTRAY_ID;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.hIcon = mainIcon;
        nid.uCallbackMessage = WM_MAINTRAY_NOTIFY;
        _tcsncpy(nid.szTip, _T("TrayEverything"), 64);
        Shell_NotifyIcon(NIM_ADD, &nid);

        HMENU trayMenu = CreatePopupMenu();
        Lang_AppendMenu(hwnd, trayMenu, MF_STRING, IDMC_MAINTRAY_RESTORE, IDS_MTMENU_RESTORE);
        AppendMenu(trayMenu, MF_SEPARATOR, 0, _T(""));
        Lang_AppendMenu(hwnd, trayMenu, MF_STRING, IDMC_MAINTRAY_SETTINGS, IDS_MTMENU_OPTIONS);
        Lang_AppendMenu(hwnd, trayMenu, MF_STRING, IDMC_MAINTRAY_UPDATE, IDS_MTMENU_UPDATE);
        Lang_AppendMenu(hwnd, trayMenu, MF_STRING, IDMC_MAINTRAY_ABOUT, IDS_MTMENU_ABOUT);
        Lang_AppendMenu(hwnd, trayMenu, MF_STRING, IDMC_MAINTRAY_EXIT, IDS_MTMENU_EXIT);
        SetProp(hwnd, MAINTRAYMENUPROP, (HANDLE)trayMenu);
    }

    MainDlg_MinimizeRestoreAnimation(hwnd, FALSE);
    ShowWindow(hwndMain, SW_HIDE);
}
void MainDlg_Switch(HWND hwnd, int newDlgId)
{
    HWND hwndMain = GetMainWindow(hwnd);
    DialogMap *dialogs = (DialogMap*)GetProp(hwndMain, DIALOGSPROP);

    if (!dialogs)
    {
        dialogs = new DialogMap;
        SetProp(hwndMain, DIALOGSPROP, (HANDLE)dialogs);
        SetProp(hwnd, CURRENTDLGPROP, (HANDLE)IDC_TB_TRAY);

        (*dialogs)[IDC_TB_TRAY] = GetWindowsList(hwnd);
        (*dialogs)[IDC_TB_SETTINGS] = CreateDialog(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_OPTIONSDLG), hwnd, OptionsDlgProc);
        (*dialogs)[IDC_TB_UPDATE] = CreateDialog(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_UPDATEDLG), hwnd, UpdateDlgProc);
        (*dialogs)[IDC_TB_ABOUT] = CreateDialog(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_ABOUTDLG), hwnd, AboutDlgProc);
    }

    int currentDlgId = (int)GetProp(hwnd, CURRENTDLGPROP);
    if (currentDlgId != newDlgId)
    {
        SetProp(hwnd, CURRENTDLGPROP, (HANDLE)newDlgId);

        HWND currentDlg = (*dialogs)[ currentDlgId ];
        HWND newDlg = (*dialogs)[ newDlgId ];

        ShowWindow(currentDlg, SW_HIDE);
        ShowWindow(newDlg, SW_SHOW);

        //Toolbar
        HWND rebar = GetDlgItem(hwnd, IDC_REBAR);
        HWND toolbar = GetDlgItem(rebar, IDC_TOOLBAR);
        ToolBar_CheckButton(toolbar, currentDlgId, FALSE);
        ToolBar_CheckButton(toolbar, newDlgId, TRUE);
    }
}
void MainDlg_EnableAeroFrame(HWND hwnd)
{
    if (pDwmIsCompositionEnabled && pDwmExtendFrameIntoClientArea)
    {
        pDwmIsCompositionEnabled(&isCompositionEnabled);
        if (isCompositionEnabled)
        {
            MARGINS mar = {0};
            mar.cyTopHeight = 72;
            
            pDwmExtendFrameIntoClientArea(hwnd, &mar);
        }
    }
}
void MainDlg_LoadAeroFunctions(HWND hwnd, BOOL pinitdialog = FALSE)
{
    static BOOL initdialog = FALSE;
    if (pinitdialog)
        initdialog = pinitdialog;
    
    if (!initdialog)
        return;
    
    if (pDwmIsCompositionEnabled && pDwmExtendFrameIntoClientArea)
        MainDlg_EnableAeroFrame(hwnd);
    else
    {
        HMODULE module = GetModuleHandle(_T("dwmapi.dll"));
        if (module)
        {
            pDwmIsCompositionEnabled = (DWMISCOMPOSITIONENABLED)GetProcAddress(module, "DwmIsCompositionEnabled");
            pDwmExtendFrameIntoClientArea = (DWMEXTENDFRAMEINTOCLIENTAREA)GetProcAddress(module, "DwmExtendFrameIntoClientArea");
            
            MainDlg_EnableAeroFrame(hwnd);
        }
    }
}
void MainDlg_OnLanguageChanged(HWND hwnd)
{
    HWND rebar = GetDlgItem(hwnd, IDC_REBAR);
    HWND toolbar = GetDlgItem(rebar, IDC_TOOLBAR);

    TBBUTTONINFO tbi = {0};
    tbi.cbSize = sizeof(TBBUTTONINFO);
    tbi.dwMask = TBIF_TEXT;

    tstring trayBtn = Lang_LoadString(hwnd, IDS_TB_TRAY);
    tbi.pszText = (LPTSTR)trayBtn.c_str();
    ToolBar_SetButtonInfo(toolbar, IDC_TB_TRAY, &tbi);

    tstring settingsBtn = Lang_LoadString(hwnd, IDS_TB_SETTINGS);
    tbi.pszText = (LPTSTR)settingsBtn.c_str();
    ToolBar_SetButtonInfo(toolbar, IDC_TB_SETTINGS, &tbi);

    tstring minimizeBtn = Lang_LoadString(hwnd, IDS_TB_MINIMIZE);
    tbi.pszText = (LPTSTR)minimizeBtn.c_str();
    ToolBar_SetButtonInfo(toolbar, IDC_TB_MINIMIZE, &tbi);

    tstring updateBtn = Lang_LoadString(hwnd, IDS_TB_UPDATE);
    tbi.pszText = (LPTSTR)updateBtn.c_str();
    ToolBar_SetButtonInfo(toolbar, IDC_TB_UPDATE, &tbi);

    tstring topmostBtn = Lang_LoadString(hwnd, IDS_TB_TOPMOST);
    tbi.pszText = (LPTSTR)topmostBtn.c_str();
    ToolBar_SetButtonInfo(toolbar, IDC_TB_TOPMOST, &tbi);

    tstring aboutBtn = Lang_LoadString(hwnd, IDS_TB_ABOUT);
    tbi.pszText = (LPTSTR)aboutBtn.c_str();
    ToolBar_SetButtonInfo(toolbar, IDC_TB_ABOUT, &tbi);

    tstring exitBtn = Lang_LoadString(hwnd, IDS_TB_EXIT);
    tbi.pszText = (LPTSTR)exitBtn.c_str();
    ToolBar_SetButtonInfo(toolbar, IDC_TB_EXIT, &tbi);


    WindowsVector *windows = Windows_Get(hwnd);
    tstring windowsCount = lstrprintf(hwnd, IDS_SB_WINDOWSCOUNT, windows->size());
    SendDlgItemMessage(hwnd, IDC_STATUSBAR, SB_SETTEXT, 1, (LPARAM)windowsCount.c_str());


    HWND hwndMain = GetMainWindow(hwnd);
    DialogMap *dialogs = (DialogMap*)GetProp(hwndMain, DIALOGSPROP);

    DialogMap::iterator iter = dialogs->begin();
    for ( ; iter != dialogs->end(); iter++)
        SendMessage(iter->second, WM_LANGUAGECHANGED, 0,0);
}

LRESULT CALLBACK RebarSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC originalProc = (WNDPROC) GetProp(hwnd, ORIGINALPROCPROP);

	switch (msg)
	{
		case WM_ERASEBKGND:
        {
            if (isCompositionEnabled)
            {
                HDC hdc = (HDC)wParam;
                RECT clientRect = {0};
                GetClientRect(hwnd, &clientRect);
                
                HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
                FillRect(hdc, &clientRect, blackBrush);
                
                return TRUE;
            }
        }
        break;
		
        case WM_DESTROY:
		{
			SetWindowLong(hwnd, GWL_WNDPROC, (LONG)originalProc);
			RemoveProp(hwnd, ORIGINALPROCPROP);
		}
		break;
	}
	
	return CallWindowProc(originalProc, hwnd, msg, wParam, lParam);
}
BOOL CALLBACK MainDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HICON mainIcon = NULL;
    static BOOL onWindowsStartup = FALSE;

    switch (msg)
    {
        default:
        {
            if (msg == externMinimizeMsg)
            {
                HWND externHwnd = (HWND)wParam;
                int index = FindWindow_FromHandle(hwnd, externHwnd);

                if (index != -1)
                {
                    WindowsVector *windows = Windows_Get(hwnd);
                    if (windows->at(index).isMinimized == FALSE)
                        Window_Minimize(hwnd, index);
                    else
                        Window_Restore(hwnd, index);
                }
            }
            else if (msg == getHookOptionsMsg)
            {
                HWND hookedWnd = (HWND)wParam;

                int index = FindWindow_FromHandle(hwnd, hookedWnd);
                if (index != -1)
                {
                    WindowsVector *windows = Windows_Get(hwnd);
                    IntVector *programOptions = Options_GetProgramOptionsVector(hwnd, windows->at(index).path);

                    BOOL sendTrayHookOptions = FALSE;
                    {
                        UINT trayHookOption = Options_GetInt(hwnd, TRAY_HOOK);
                        if (trayHookOption == HO_MINBUTTON)
                        {
                            if (!programOptions->at(PROGRAMOPTION_NOHOOKMINBUTTON))
                                sendTrayHookOptions = TRUE;
                        }
                        else if (trayHookOption == HO_TRAYBUTTON)
                        {
                            if (!programOptions->at(PROGRAMOPTION_NOTRAYBUTTON))
                                sendTrayHookOptions = TRUE;
                        }
                        else
                            sendTrayHookOptions = TRUE;
                    }

                    if (sendTrayHookOptions)
                        SendMessage(hookedWnd, setHookOptionsMsg, TRAY_HOOK, Options_GetInt(hwnd, TRAY_HOOK));
                    else
                    {
                        //Let the hooked window reset its settings (using Reset())
                        SendMessage(hookedWnd, setHookOptionsMsg, TRAY_HOOK, 0);
                    }
                }
            }
            else if (msg == setHookOptionsMsg)
            {
                if (wParam == PROGRAMOPTION_NOHOOKMINBUTTON)
                {
                    //Add the hooked window to the minimize list of programs
                    HWND hookedWnd = (HWND)lParam;

                    int index = FindWindow_FromHandle(hwnd, hookedWnd);
                    if (index != -1)
                    {
                        WindowsVector *windows = Windows_Get(hwnd);

                        IntVector *programOptions = Options_GetProgramOptionsVector(hwnd, windows->at(index).path);
                        programOptions->at(PROGRAMOPTION_NOHOOKMINBUTTON) =
                            !programOptions->at(PROGRAMOPTION_NOHOOKMINBUTTON);
                    }
                }
            }
            //Message sent when the taskbar is created (ie: on crash)
            else if (msg == taskbarCreatedMsg)
            {
                //Restore the tray icons if needed
                if (!Options_GetInt(hwnd, TRAY_NOICON))
                {
                    WindowsVector *windows = Windows_Get(hwnd);
                    for (int i = 0; i<windows->size(); i++)
                    {
                        if (windows->at(i).isMinimized)
                            TrayIcon_Set(hwnd, i);
                    }
                }

                //Restore TrayEverything icon if it's tray-ed
                if (!IsWindowVisible(hwnd))
                    if (!Options_GetInt(hwnd, MAINTRAY_HIDEONMIN))
                        MainDlg_Minimize(hwnd, mainIcon);
            }
        }
        break;
        
        case WM_PAINT:
        {
            if (isCompositionEnabled)
            {
                PAINTSTRUCT ps = {0};
                HDC hdc = BeginPaint(hwnd, &ps);
                
                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                
                HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
                FillRect(hdc, &clientRect, blackBrush);
                
                EndPaint(hwnd, &ps);
            }
        }
        break;
        
        case WM_DWMCOMPOSITIONCHANGED:
            MainDlg_LoadAeroFunctions(hwnd);
            break;
        
        case WM_LANGUAGECHANGED:
            MainDlg_OnLanguageChanged(hwnd);
            break;

        case WM_LBUTTONDOWN:
            MainDlg_MoveOnClick(hwnd);
            break;

        case WM_NOTIFY:
        {
            NMHDR *nmh = (NMHDR*)lParam;
            switch(nmh->code)
            {
                case NM_CUSTOMDRAW:
                {
                    if (nmh->idFrom != IDC_TOOLBAR)
                        break;
                    if (!isCompositionEnabled)
                        break;
                    
                    LPNMTBCUSTOMDRAW tbcd = (LPNMTBCUSTOMDRAW)lParam;
                    switch(tbcd->nmcd.dwDrawStage)
                    {
                        case CDDS_ITEMPREPAINT:
                        {
                            tbcd->clrText = RGB(100,100,100);
                            tbcd->clrTextHighlight = RGB(100,100,100);
                            tbcd->clrBtnFace = RGB(100,100,100);
                            tbcd->clrBtnHighlight = RGB(100,100,100);
                            
                            SetWindowLong(hwnd, DWL_MSGRESULT, TBCDRF_USECDCOLORS);
                        }
                        return TBCDRF_USECDCOLORS;
                        
                        case CDDS_PREPAINT:
                            SetWindowLong(hwnd, DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                            return CDRF_NOTIFYITEMDRAW;
                    }
                }
                break;
                
                case TBN_HOTITEMCHANGE:
                {
                    LPNMTBHOTITEM nmhi = (LPNMTBHOTITEM)lParam;
                    if (nmhi->hdr.idFrom != IDC_TOOLBAR)
                        break;

                    tstring statusBarText = _T("");
                    switch(nmhi->idNew)
                    {
                        case IDC_TB_TRAY:
                            statusBarText = Lang_LoadString(hwnd, IDS_SB_TRAY);
                            break;
                        case IDC_TB_SETTINGS:
                            statusBarText = Lang_LoadString(hwnd, IDS_SB_SETTINGS);
                            break;
                        case IDC_TB_MINIMIZE:
                            statusBarText = Lang_LoadString(hwnd, IDS_SB_MINIMIZE);
                            break;
                        case IDC_TB_UPDATE:
                            statusBarText = Lang_LoadString(hwnd, IDS_SB_UPDATE);
                            break;
                        case IDC_TB_TOPMOST:
                            statusBarText = Lang_LoadString(hwnd, IDS_SB_TOPMOST);
                            break;
                        case IDC_TB_ABOUT:
                            statusBarText = Lang_LoadString(hwnd, IDS_SB_ABOUT);
                            break;
                        case IDC_TB_EXIT:
                            statusBarText = Lang_LoadString(hwnd, IDS_SB_EXIT);
                            break;
                        default:
                            statusBarText = Lang_LoadString(hwnd, IDS_SB_READY);
                            break;
                    }

                    // To avoid flickering
                    SendDlgItemMessage(hwnd, IDC_STATUSBAR, SB_SETTEXT, 0, (LPARAM)statusBarText.c_str());
                }
                break;

                case LVN_COLUMNCLICK:
                    WindowsList_OnColumnClick((LPNMLISTVIEW)nmh);
                    break;
                case LVN_HOTTRACK:
                    WindowsList_OnHotTrack((LPNMLISTVIEW)nmh);
                    break;

                case NM_CLICK:
                {
                    switch(nmh->idFrom)
                    {
                        case IDC_WINDOWSLIST:
                            WindowsList_OnLeftClick((LPNMITEMACTIVATE)nmh);
                            break;
                    }
                }
                break;
                case NM_DBLCLK:
                {
                    switch(nmh->idFrom)
                    {
                        case IDC_WINDOWSLIST:
                            WindowsList_OnDoubleClick((LPNMITEMACTIVATE)nmh);
                            break;
                    }
                }
                break;
                case NM_RCLICK:
                {
                    switch(nmh->idFrom)
                    {
                        case IDC_WINDOWSLIST:
                            WindowsList_OnRightClick((LPNMITEMACTIVATE)nmh);
                            break;
                    }
                }
                break;
            }
        }
        break;

        case WM_MAINTRAY_NOTIFY:
        {
            switch(lParam)
            {
                case WM_LBUTTONDBLCLK:
                {
                    onWindowsStartup = FALSE;
                    MainDlg_Restore(hwnd);
                }
                break;

                case WM_RBUTTONUP:
                {
                    POINT pt; GetCursorPos(&pt);
                    SetForegroundWindow(hwnd);

                    //The notifications are received by this proc (through WM_COMMAND)
                    HMENU trayMenu = (HMENU)GetProp(hwnd, MAINTRAYMENUPROP);
                    TrackPopupMenuEx(trayMenu, TPM_LEFTALIGN|TPM_BOTTOMALIGN|TPM_LEFTBUTTON,
                        pt.x,pt.y, hwnd, NULL);
                }
                break;
            }
        }
        break;

        case WM_PROGTRAY_NOTIFY:
        {
            if (wParam == 0)
                break;

            WindowsVector *windows = Windows_Get(hwnd);

            int index = FindWindow_FromUniqueId(hwnd, wParam);
            if (index != -1)
            {
                switch(lParam)
                {
                    case WM_LBUTTONDBLCLK:
                        Window_Restore(hwnd, index);
                        break;

                    case WM_RBUTTONUP:
                    {
                        POINT pt; GetCursorPos(&pt);
                        SetForegroundWindow(hwnd);

                        UINT cmd = TrackPopupMenuEx(windows->at(index).trayMenu,
                            TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD,
                            pt.x,pt.y, hwnd, NULL);

                        if(cmd == IDMC_PROGTRAY_RESTORE)
                            Window_Restore(hwnd, index);
                    }
                    break;
                }
            }
            else
            {
                //Might be a group id, search and get all windows
                std::vector<int> windowIndexes;
                for (int i = 0; i<windows->size(); i++)
                {
                    if (wParam == windows->at(i).uniqueGroupId)
                    {
                        int index = FindWindow_FromUniqueId(hwnd, windows->at(i).uniqueId);
                        if (index != -1)
                            windowIndexes.push_back(index);
                    }
                }

                if (windowIndexes.size())
                {
                    switch(lParam)
                    {
                        case WM_LBUTTONDBLCLK:
                        {
                            for (int i = 0; i<windowIndexes.size(); i++)
                                Window_Restore(hwnd, windowIndexes[i]);
                        }
                        break;

                        case WM_RBUTTONUP:
                        {
                            POINT pt; GetCursorPos(&pt);
                            SetForegroundWindow(hwnd);

                            //Put together all the menus
                            HMENU trayMenu = CreatePopupMenu();
                            for (int i = 0; i<windowIndexes.size(); i++)
                            {
                                int index = windowIndexes[i];

                                TCHAR menuItemText[256] = _T("");

                                MENUITEMINFO mii = {0};
                                mii.cbSize = sizeof(MENUITEMINFO);
                                mii.fMask = MIIM_TYPE|MIIM_STATE|MIIM_ID;
                                mii.dwTypeData = menuItemText;
                                mii.cch = 256;
                                GetMenuItemInfo(windows->at(index).trayMenu, IDMC_PROGTRAY_RESTORE, FALSE, &mii);

                                mii.wID = windows->at(index).uniqueId;
                                InsertMenuItem(trayMenu, 0, TRUE, &mii);
                            }

                            UINT cmd = TrackPopupMenuEx(trayMenu,
                                TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD,
                                pt.x,pt.y, hwnd, NULL);

                            for (int i = 0; i<windowIndexes.size(); i++)
                            {
                                int index = windowIndexes[i];
                                if (cmd == windows->at(index).uniqueId)
                                    Window_Restore(hwnd, index);
                            }

                            DestroyMenu(trayMenu);
                        }
                        break;
                    }
                }
            }
        }
        break;

        case WM_TIMER:
        {
            switch(wParam)
            {
                case OPENWINDOWS_TIMER_ID:
                {
                    OpenWindows_Check(hwnd);
                    EnumWindows(OpenWindows_EnumProc, (LPARAM)hwnd);
                }
                break;

                case ACTIVEWINDOWS_TIMER_ID:
                {
                    if (Options_GetInt(hwnd, TRAY_AUTOMIN))
                    {
                        int time = Options_GetInt(hwnd, TRAY_AUTOMIN_TIME);
                        switch(Options_GetInt(hwnd, TRAY_AUTOMIN_TIMETYPE))
                        {
                            //Nothing to do
                            default:
                            case TT_SECONDS:
                                break;

                            case TT_MINUTES:
                                time *= 60;
                                break;
                        }

                        HWND foreground = GetForegroundWindow();

                        WindowsVector *windows = Windows_Get(hwnd);
                        for (int i = 0; i<windows->size(); i++)
                        {
                            IntVector *options = Options_GetProgramOptionsVector(hwnd, windows->at(i).path);

                            if (options->at(PROGRAMOPTION_NOAUTOMINIMIZE) == TRUE)
                                windows->at(i).secondsInactive = 0;
                            else if (windows->at(i).isMinimized)
                                windows->at(i).secondsInactive = 0;
                            else
                            {
                                if (foreground == windows->at(i).hwnd)
                                    windows->at(i).secondsInactive = 0;
                                else
                                    windows->at(i).secondsInactive++;

                                if (windows->at(i).secondsInactive == time)
                                    Window_Minimize(hwnd, i);
                            }
                        }
                    }
                }
                break;
            }
        }
        break;

        case WM_MENUCHAR:
        {
            TCHAR keypressed = LOWORD(wParam);
            UINT cmdId = 0;

            HWND toolbar = GetDlgItem(hwnd, IDC_TOOLBAR);
            if (SendMessage(toolbar, TB_MAPACCELERATOR, (WPARAM)keypressed, (LPARAM)&cmdId))
            {
                //Feature in Windows.  TB_MAPACCELERATOR matches either the mnemonic
                //character or the first character in a tool item.  This behavior is
                //undocumented and unwanted.  The fix is to ensure that the tool item
                //contains a mnemonic when TB_MAPACCELERATOR returns true.
                SendMessage(hwnd, WM_COMMAND, cmdId, TRUE);
            }
        }
        break;

        case WM_CHECKUPDTHREAD_END:
        {
            RemoveProp(hwnd, SILENTUPDATEPROP);
            HANDLE checkUpdatesThread = RemoveProp(hwnd, CHECKUPDTHREADPROP);
            WaitForSingleObject(checkUpdatesThread, INFINITE);
            CloseHandle(checkUpdatesThread);

            HWND toolbar = GetDlgItem(hwnd, IDC_TOOLBAR);
            UpdatesVector *updates = Updates_Get(hwnd);
            if (updates)
            {
                POINT pt = {0};
                HWND parent = NULL;

                if (updates->size())
                {
                    if (IsWindowVisible(hwnd))
                    {
                        HWND toolbar = GetDlgItem(hwnd, IDC_TOOLBAR);

                        RECT rc;
                        ToolBar_GetRect(toolbar, IDC_TB_UPDATE, &rc);
                        MapWindowPoints(toolbar, NULL, (LPPOINT)&rc, 2);

                        //Let's display the balloon under the bitmap...
                        pt.x = (rc.left+rc.right)/2;
                        pt.y = (rc.top+rc.bottom)/2 +30;

                        parent = hwnd;
                    }
                    else
                    {
                        HWND tray = GetTrayWindow();

                        if (tray)
                        {
                            //Get an ID of the parent process for system tray
                            DWORD processID = 0;
                            GetWindowThreadProcessId(tray, &processID);
                            HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, 0, processID);

                            //Check how many buttons are there - should be more than 0
                            int buttonsCount = SendMessage(tray, TB_BUTTONCOUNT, 0, 0);

                            LPVOID data = VirtualAllocEx(process, NULL,
                                sizeof(TBBUTTON), MEM_COMMIT, PAGE_READWRITE);

                            BOOL iconFound = FALSE;
                            RECT iconRect = {0};
                            for(int index = 0; index<buttonsCount; index++)
                            {
                                DWORD bytesRead = 0;

                                //first let's read TBUTTON information
                                //about each button in a task bar of tray
                                SendMessage(tray, TB_GETBUTTON, index, (LPARAM)data);

                                //We filled lpData with details of iButton icon of toolbar
                                //- now let's copy this data from tray application
                                //back to our process
                                TBBUTTON buttonData = {0};
                                ReadProcessMemory(process, data, &buttonData,
                                    sizeof(TBBUTTON), &bytesRead);

                                //Let's read extra data of each button:
                                //there will be a HWND of the window that
                                //created an icon and icon ID
                                DWORD extraData[2] = {0,0};
                                ReadProcessMemory(process, (LPVOID)buttonData.dwData,
                                    extraData, sizeof(extraData), &bytesRead);

                                HWND iconOwner = (HWND)extraData[0];
                                int iconId = (int)extraData[1];

                                if(iconOwner == hwnd || iconId == MAINTRAY_ID)
                                {
                                    //We found our icon - in WinXP it could be hidden - let's check it
                                    if((buttonData.fsState & TBSTATE_HIDDEN) != TBSTATE_HIDDEN)
                                    {
                                        //Ask a tool bar of rectangle of our icon
                                        SendMessage(tray, TB_GETITEMRECT, index, (LPARAM)data);
                                        ReadProcessMemory(process, data, &iconRect, sizeof(RECT), &bytesRead);

                                        MapWindowPoints(tray, NULL, (LPPOINT)&iconRect, 2);

                                        iconFound = TRUE;
                                        break;
                                    }
                                }
                            }

                            VirtualFreeEx(process, data, 0, MEM_RELEASE);
                            CloseHandle(process);

                            if (iconFound)
                            {
                                SetForegroundWindow(hwnd);

                                pt.x = iconRect.left+8;
                                pt.y = iconRect.top+8;

                                parent = GetShellWindow();
                            }
                        }
                    }

                    tstring balloonTitle = Lang_LoadString(hwnd, IDS_UPDBALLOON_TITLE);
                    tstring balloonText = Lang_LoadString(hwnd, IDS_UPDBALLOON_TEXT);

                    //Display the balloon
                    BalloonHelp *bh = new BalloonHelp;
                    MSG clickInfo = { hwnd, WM_COMMAND, MAKELONG(IDMC_MAINTRAY_UPDATE,0), 0 };
                    bh->Create(balloonTitle.c_str(), balloonText.c_str(), pt, MAKEINTRESOURCE(IDI_MAINICON),
                        BHO_SHOW_CLOSEBUTTON, parent, &clickInfo, 0);

                    Updates_Remove(hwnd);
                }
            }

            TBBUTTONINFO tbi = {0};
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_TEXT;
            tbi.pszText = (LPTSTR)Lang_LoadString(hwnd, IDS_TB_UPDATE).c_str();
            ToolBar_SetButtonInfo(toolbar, IDC_TB_UPDATE, &tbi);
        }
        break;

        case WM_INITDIALOG:
        {
            MainDlg_LoadAeroFunctions(hwnd, TRUE);

            externMinimizeMsg = RegisterWindowMessage(WM_TE_MINIMIZE);
            taskbarCreatedMsg = RegisterWindowMessage(_T("TaskbarCreated"));
            getHookOptionsMsg = RegisterWindowMessage(WM_TE_GETHOOKOPTIONS);
            setHookOptionsMsg = RegisterWindowMessage(WM_TE_SETHOOKOPTIONS);
            updateHookOptionsMsg = RegisterWindowMessage(WM_TE_UPDATEHOOKOPTIONS);
            onWindowsStartup = (BOOL)lParam;

            HBITMAP ssBmp = LoadImageFromResource(IDB_SPLASHSCREEN, _T("GIF"));

            COLORREF transparentColor = 0;
            if (IsWindowsXp())
                transparentColor = RGB(255,24,255);
            else
                transparentColor = RGB(255,0,255);

            SplashScreen ss;
            ss.Create(hwnd, _T("TrayEverything"), 0, CSS_CENTERSCREEN);
            ss.SetBitmap(ssBmp, GetRValue(transparentColor), GetGValue(transparentColor), GetBValue(transparentColor));
            ss.SetTextFont(_T("MS Shell Dlg"), 16, CSS_TEXT_NORMAL);
            ss.SetTextRect(610,244,130,20);
            ss.SetTextColor( RGB(255,255,255) );
            ss.SetTextFormat(DT_SINGLELINE|DT_CENTER|DT_VCENTER);

	ss.SetText( _T("Loading...") );
	ss.Show();

            Windows_Create(hwnd);
            Options_Load(hwnd);
            Lang_Load(hwnd);

    Sleep(300);
    ss.SetText( Lang_LoadString(hwnd, IDS_SS_CHECKINGMUTEX).c_str() );

            //mutex, impedisce di avviare due volte lo stesso programma
            CreateMutex(NULL, TRUE, _T("TrayEverything"));
            if(GetLastError() == ERROR_ALREADY_EXISTS)
            {
                MessageBox(hwnd, _T("TrayEverything is already running. ")
                    _T("If you don't see it, check the system tray."),
                    _T("TrayEverything"), MB_OK|MB_TOPMOST);

                HWND teWnd =  FindWindow(NULL, _T("TrayEverything 1.1"));
                if (IsWindowVisible(teWnd))
                    SetForegroundWindow(teWnd);
                else
                    SendMessage(teWnd, WM_MAINTRAY_NOTIFY, 0, WM_LBUTTONDBLCLK);

                DestroyWindow(hwnd);
                return 0;
            }

    Sleep(300);
    ss.SetText( Lang_LoadString(hwnd, IDS_SS_INTERFACE).c_str() );

            //
            mainIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAINICON));
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)mainIcon);
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)mainIcon);
            
            //HWND toolBar = GetDlgItem(hwnd, IDC_TOOLBAR);
            HWND reBar = GetDlgItem(hwnd, IDC_REBAR);
            HWND listView = GetDlgItem(hwnd, IDC_WINDOWSLIST);
            HWND statusBar = GetDlgItem(hwnd, IDC_STATUSBAR);
            //MainToolbar_Init(toolBar);
            MainToolbar_Init(hwnd);
            WindowsList_Init(listView);
            StatusBar_Init(statusBar);
            Rebar_Init(reBar);
            
            //Set the timer to refresh open windows
            SetTimer(hwnd, OPENWINDOWS_TIMER_ID, 100, NULL);
            
            //
            MainDlg_Switch(hwnd, IDC_TB_TRAY);
            ShowWindow(hwnd, SW_SHOW);

    Sleep(300);
    ss.SetText( Lang_LoadString(hwnd, IDS_SS_OPTIONS).c_str() );

            if (onWindowsStartup)
                MainDlg_Minimize(hwnd, mainIcon);

            Options_Restore(hwnd);

    Sleep(300);
    ss.Hide();
        }
        break;

        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS FAR* lpwndpos = (WINDOWPOS FAR*)lParam;
            if (onWindowsStartup)
                lpwndpos->flags &= ~SWP_SHOWWINDOW;
        }
        break;

        case WM_HOTKEY:
        {
            switch(wParam)
            {
                case MINHOTKEY_ID:
                {
                    HWND hwndForeground = GetForegroundWindow();
                    int index = FindWindow_FromHandle(hwnd, hwndForeground);
                    if (index == -1) break;

                    Window_Minimize(hwnd, index);
                }
                break;

                case TEHOTKEY_ID:
                    MainDlg_Restore(hwnd);
                    break;
            }
        }
        break;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            //Main tray menu
                case IDMC_MAINTRAY_RESTORE:
                case IDMC_MAINTRAY_SETTINGS:
                case IDMC_MAINTRAY_UPDATE:
                case IDMC_MAINTRAY_ABOUT:
                {
                    onWindowsStartup = FALSE;
                    MainDlg_Restore(hwnd);

                    switch(LOWORD(wParam))
                    {
                        case IDMC_MAINTRAY_SETTINGS:
                            MainDlg_Switch(hwnd, IDC_TB_SETTINGS);
                            break;
                        case IDMC_MAINTRAY_UPDATE:
                            MainDlg_Switch(hwnd, IDC_TB_UPDATE);
                            break;
                        case IDMC_MAINTRAY_ABOUT:
                            MainDlg_Switch(hwnd, IDC_TB_ABOUT);
                            break;
                    }
                }
                break;

                case IDMC_MAINTRAY_EXIT:
                    PostMessage(hwnd, WM_CLOSE, TRUE, 0);
                    break;
            //end main tray

                case IDC_TB_TRAY:
                    MainDlg_Switch(hwnd, IDC_TB_TRAY);
                    break;

                case IDC_TB_MINIMIZE:
                    MainDlg_Minimize(hwnd, mainIcon);
                    break;

                case IDC_TB_SETTINGS:
                    MainDlg_Switch(hwnd, IDC_TB_SETTINGS);
                    break;

                case IDC_TB_UPDATE:
                {
                    MainDlg_Switch(hwnd, IDC_TB_UPDATE);

                    TBBUTTONINFO tbi = {0};
                    tbi.cbSize = sizeof(TBBUTTONINFO);
                    tbi.dwMask = TBIF_TEXT;
                    tbi.pszText = (LPTSTR)Lang_LoadString(hwnd, IDS_TB_UPDATE).c_str();

                    HWND toolbar = GetDlgItem(hwnd, IDC_TOOLBAR);
                    ToolBar_SetButtonInfo(toolbar, IDC_TB_UPDATE, &tbi);
                }
                break;

                case IDC_TB_TOPMOST:
                {
                    HWND toolbar = GetDlgItem(hwnd, IDC_TOOLBAR);
                    if (ToolBar_IsButtonChecked(toolbar, IDC_TB_TOPMOST))
                    {
                        SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

                        if (lParam == TRUE)
                            ToolBar_CheckButton(toolbar, IDC_TB_TOPMOST, FALSE);
                    }
                    else
                    {
                        SetWindowPos(hwnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

                        if (lParam == TRUE)
                            ToolBar_CheckButton(toolbar, IDC_TB_TOPMOST, TRUE);
                    }
                }
                break;

                case IDC_TB_ABOUT:
                    MainDlg_Switch(hwnd, IDC_TB_ABOUT);
                    break;

                case IDC_TB_EXIT:
                    PostMessage(hwnd, WM_CLOSE, TRUE, 0);
                    break;
			}
        }
        break;

	    case WM_CLOSE:
        {
            if (wParam != TRUE)
            {
                if (Options_GetInt(hwnd, MAINTRAY_MINONCLOSE))
                {
                    SendMessage(hwnd, WM_COMMAND, MAKELONG(IDC_TB_MINIMIZE, 0), 0);
                    break;
                }
            }

            if (GetProp(hwnd, CHECKUPDTHREADPROP))
            {
                ErrorBox(hwnd, FALSE, IDS_ERR_CHECKINGFORUPDATES);
                break;
            }

            if (TrayIcon_RemoveAll(hwnd) == FALSE)
                break;

            MainDlg_Restore(hwnd);
			EndDialog(hwnd,0);
        }
        break;

        case WM_DESTROY:
        {
            Hook_Stop();
            DestroyIcon(mainIcon);
            Windows_Destroy(hwnd);
            Options_Save(hwnd);
            Options_Destroy(hwnd);
            MainToolbar_Free(hwnd);
            
            DialogMap *dialogs = (DialogMap*)RemoveProp(hwnd, DIALOGSPROP);
            delete dialogs;

            UnregisterHotKey(hwnd, MINHOTKEY_ID);
        }
        break;
    }

    return FALSE;
}

int WINAPI WinMain (HINSTANCE instance, HINSTANCE prevInstance, LPSTR lpszArgument, int showComand)
{
    RegisterDialogClass(_T("TrayEverythingDialog"));
    
    INITCOMMONCONTROLSEX iccs = {0};
    iccs.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccs.dwICC = ICC_USEREX_CLASSES|ICC_STANDARD_CLASSES|ICC_WIN95_CLASSES;
    InitCommonControlsEx(&iccs);
    
    tstring arguments = GetCommandLine();
    BOOL onWindowsStart = (arguments.find(_T("/windowsstart")) != tstring::npos);

    return DialogBoxParam(instance, MAKEINTRESOURCE(IDD_MAINDLG), NULL, MainDlgProc, onWindowsStart);
}
