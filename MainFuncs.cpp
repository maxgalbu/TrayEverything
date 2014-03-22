#include "resource.h"
#include <wininet.h>
#include "CommonFuncs.h"
#include "LangFuncs.h"
#include "OptionsFuncs.h"

#include "MainFuncs.h"

void MainDlg_MoveOnClick(HWND hwnd)
{
    HWND hwndMain = GetMainWindow(hwnd);
    
    POINT pt; GetCursorPos(&pt);
    ScreenToClient(hwndMain, &pt);
    
    PostMessage(hwndMain, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(pt.x, pt.y));
}

UpdatesVector *Updates_Get(HWND hwnd)
{
    return (UpdatesVector*)GetProp(hwnd, UPDATESPROP);
}
void Updates_Remove(HWND hwnd)
{
    UpdatesVector *updatesVec = (UpdatesVector*)RemoveProp(hwnd, UPDATESPROP);
    delete updatesVec;
}
void Updates_Set(HWND hwnd, UpdatesVector *updates)
{
    if (Updates_Get(hwnd))
        Updates_Remove(hwnd);
    
    SetProp(hwnd, UPDATESPROP, (HANDLE)updates);
}

void TrayIcon_CreateMenu(HWND hwnd, int index)
{
    WindowsVector *windows = Windows_Get(hwnd);
    if (windows->at(index).trayMenu)
        DestroyMenu(windows->at(index).trayMenu);
    
    tstring menuItemText = _T("");
    if (Options_GetInt(hwnd, TRAY_GROUPICON))
        menuItemText = windows->at(index).windowText;
    else
        menuItemText = windows->at(index).filename;
    
    HMENU trayMenu = CreatePopupMenu();
    AppendMenu(trayMenu, MF_STRING, IDMC_PROGTRAY_RESTORE, 
        lstrprintf(hwnd, IDS_TMENU_RESTORE, menuItemText.c_str()).c_str());
    windows->at(index).trayMenu = trayMenu;
}
void TrayIcon_Set(HWND hwnd, int index)
{
    WindowsVector *windows = Windows_Get(hwnd);
    
    BOOL groupIcon = Options_GetInt(hwnd, TRAY_GROUPICON);
    if (groupIcon)
        if (windows->at(index).uniqueGroupId == 0)
            windows->at(index).uniqueGroupId = GetUniqueId();
    
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = GetMainWindow(hwnd);
    nid.uID = groupIcon ? 
        windows->at(index).uniqueGroupId : 
        windows->at(index).uniqueId;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.hIcon = Window_GetIcon(hwnd, index);
    nid.uCallbackMessage = WM_PROGTRAY_NOTIFY;
    
    tstring tooltip = _T("");
    {
        if (groupIcon)
            tooltip = windows->at(index).filename;
        else
            tooltip = windows->at(index).windowText;
        
        if (tooltip.size() > 64)
            tooltip = tooltip.substr(0,60) + _T("...");
    }
    _tcscpy(nid.szTip, tooltip.c_str());
    
    Shell_NotifyIcon(NIM_ADD, &nid);
    
    //Create the menu
    TrayIcon_CreateMenu(hwnd, index);
}
void TrayIcon_Remove(HWND hwnd, int index)
{
    WindowsVector *windows = Windows_Get(hwnd);
    
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = GetMainWindow(hwnd);
    
    if (!Options_GetInt(hwnd, TRAY_GROUPICON))
        nid.uID = windows->at(index).uniqueId;
    else
    {
        //If other windows still share the same icon, don't remove it
        BOOL windowsGroupCount = 0;
        
        unsigned long uniqueGroupId = windows->at(index).uniqueGroupId;
        for (int i = 0; i<windows->size(); i++)
        {
            if (uniqueGroupId == windows->at(i).uniqueGroupId)
                windowsGroupCount++;
        }
        
        //If there's only one window left, remove the icon
        if (windowsGroupCount == 1)
            nid.uID = uniqueGroupId;
    }
    
    if (nid.uID)
    {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        DestroyMenu(windows->at(index).trayMenu);
    }
}
BOOL TrayIcon_RemoveAll(HWND hwnd, BOOL restore)
{
    WindowsVector *windows = Windows_Get(hwnd);
    for (int i = 0; i < windows->size(); i++)
    {
        if (windows->at(i).isMinimized)
        {
            if (restore)
            {
                if (Options_GetInt(hwnd, TRAY_PWDPROTECT) && windows->at(i).password != _T(""))
                {
                    int ret = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_GETPASSWORDDLG), 
                        hwnd, GetPasswordDlgProc, i+1);
                    
                    if (ret == IDCANCEL)
                        return FALSE;
                }
                
                ShowWindow(windows->at(i).hwnd, SW_SHOW);
            }
            
            TrayIcon_Remove(hwnd, i);
            
            windows->at(i).uniqueGroupId = 0;
            if (restore)
            {
                windows->at(i).isMinimized = FALSE;
                windows->at(i).toUpdate |= WTU_VISIBLE;
            }
        }
    }
    
    return TRUE;
}

HICON Window_GetIcon(HWND hwnd, int index)
{
    WindowsVector *windows = Windows_Get(hwnd);
    
    HICON progIcon = NULL;
    if (!progIcon)
        progIcon = (HICON)SendMessage(windows->at(index).hwnd, WM_GETICON, ICON_BIG, 0);
    if (!progIcon)
        progIcon = ExtractIcon(GetModuleHandle(NULL), windows->at(index).path.c_str(), 0);
    if (!progIcon)
        progIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    return progIcon;
}
BOOL CALLBACK Window_MinimizeRestoreChildWindowsProc(HWND hwnd, LPARAM param)
{
    MINIMIZERESTORECHILDWINDOWSSTRUCT *mrcws = (MINIMIZERESTORECHILDWINDOWSSTRUCT*)param;
    
    BOOL isPopup = GetWindowLong(hwnd, GWL_STYLE) & WS_POPUP;
    BOOL isToolWindow = GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW;
    if (isPopup || isToolWindow)
    {
        DWORD processID = 0;
        GetWindowThreadProcessId(hwnd, &processID);
        
        if (processID == mrcws->processID)
        {
            if (mrcws->restore)
            {
                if (BOOL(GetProp(hwnd, _T("TrayEverythingAlreadyHidden"))) == FALSE)
                {
                    RemoveProp(hwnd, _T("TrayEverythingAlreadyHidden"));
                    ShowWindow(hwnd, SW_SHOW);
                }
            }
            else
            {
                if (IsWindowVisible(hwnd))
                    ShowWindow(hwnd, SW_HIDE);
                else
                    SetProp(hwnd, _T("TrayEverythingAlreadyHidden"), (HANDLE)TRUE);
            }
        }
    }
    
    return TRUE;
}
void Window_Restore(HWND hwnd, int index)
{
    WindowsVector *windows = Windows_Get(hwnd);
    
    if (Options_GetInt(hwnd, TRAY_PWDPROTECT) && windows->at(index).password != _T(""))
    {
        int ret = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_GETPASSWORDDLG), 
            hwnd, GetPasswordDlgProc, index+1);
        
        if (ret == IDCANCEL)
            return;
    }
    
    TrayIcon_Remove(hwnd, index);
    
    //Show the selected window
    ShowWindow(windows->at(index).hwnd, SW_SHOW);
    SetForegroundWindow(windows->at(index).hwnd);
    
    //Show all the child windows with WS_EX_TOOLWINDOW extrastyle
    MINIMIZERESTORECHILDWINDOWSSTRUCT mrcws = {windows->at(index).processID, TRUE };
    EnumWindows(Window_MinimizeRestoreChildWindowsProc, (LPARAM)&mrcws);
    
    windows->at(index).uniqueGroupId = 0;
    windows->at(index).isMinimized = FALSE;
    windows->at(index).toUpdate |= WTU_VISIBLE;
}
void Window_Minimize(HWND hwnd, int index)
{
    WindowsVector *windows = Windows_Get(hwnd);
    
    IntVector *options = Options_GetProgramOptionsVector(hwnd, windows->at(index).path);
    if (options->at(PROGRAMOPTION_NEVERMINIMIZE) == TRUE)
        return;
    
    if (Options_GetInt(hwnd, TRAY_PWDPROTECT))
    {
        int ret = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETPASSWORDDLG), 
            hwnd, SetPasswordDlgProc, index+1);
        
        if (ret == IDCANCEL)
            return;
    }
    
    if (!Options_GetInt(hwnd, TRAY_NOICON))
    {
        if (!Options_GetInt(hwnd, TRAY_GROUPICON))
            TrayIcon_Set(hwnd, index);
        else
        {
            BOOL found = FALSE;
            for (int i = 0; i<windows->size(); i++)
            {
                if (windows->at(i).isMinimized)
                {
                    if (windows->at(index).path == windows->at(i).path)
                    {
                        windows->at(index).uniqueGroupId = windows->at(i).uniqueGroupId;
                        TrayIcon_CreateMenu(hwnd, index);
                        found = TRUE;
                    }
                }
            }
            
            if (!found)
                TrayIcon_Set(hwnd, index);
        }
    }
    
    //Hide the selected window
    ShowWindow(windows->at(index).hwnd, SW_HIDE);
    
    //Hide all the windows with WS_EX_TOOLWINDOW extrastyle
    MINIMIZERESTORECHILDWINDOWSSTRUCT mrcws = {windows->at(index).processID, FALSE };
    EnumWindows(Window_MinimizeRestoreChildWindowsProc, (LPARAM)&mrcws);
    
    windows->at(index).isMinimized = TRUE;
    windows->at(index).toUpdate |= WTU_VISIBLE;
}


BOOL IsDayGone(HWND hwnd)
{
    int currTime = time (NULL)/86400;
    int lastUpdateTime = Options_GetInt(hwnd, UPDATES_LAST_DATE);
    
    if (lastUpdateTime < currTime) 
        return TRUE;
    return FALSE;
}
BOOL IsWeekGone(HWND hwnd)
{
    int currTime = time (NULL)/604800;
    int lastUpdateTime = Options_GetInt(hwnd, UPDATES_LAST_DATE);
    
    if (lastUpdateTime < currTime) 
        return TRUE;
    return FALSE;
}   

void SaveLastUpdateDayDate(HWND hwnd)
{
    int timeNow = time (NULL)/86400;
    Options_SetInt(hwnd, UPDATES_LAST_DATE, timeNow);
}
void SaveLastUpdateWeekDate(HWND hwnd)
{
    int timeNow = time (NULL)/604800;
    Options_SetInt(hwnd, UPDATES_LAST_DATE, timeNow);
}

int FindWindow_FromUniqueId(HWND hwnd, unsigned long uniqueId)
{
    WindowsVector *windows = Windows_Get(hwnd);
	for (int i = 0; i<windows->size(); i++)
    {
        if(uniqueId == windows->at(i).uniqueId)
            return i;
    }
    
    return -1;
}
int FindWindow_FromHandle(HWND hwnd, HWND window)
{
    WindowsVector *windows = Windows_Get(hwnd);
	for (int i = 0; i<windows->size(); i++)
    {
        if(window == windows->at(i).hwnd)
            return i;
    }
    
    return -1;
}

BOOL CALLBACK GetPasswordDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int index = 0;
    
    switch (msg)
    {
        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;
        
        case WM_INITDIALOG:
        {            
            index = lParam-1;
            
            Lang_WindowText(hwnd, IDS_GP_WINDOW);
            Lang_DlgItemText(hwnd, IDOK, IDS_DLG_OK);
            Lang_DlgItemText(hwnd, IDCANCEL, IDS_DLG_CANCEL);
            Lang_DlgItemText(hwnd, IDC_GP_STATUS, IDS_SGP_ST_ENTERPWD);
            
            SendDlgItemMessage(hwnd, IDC_GP_PWD, EM_SETLIMITTEXT, 25, 0);
            SetForegroundWindow(hwnd);
        }
        break;
        
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
                
                case IDOK:
                {
                    TCHAR password[100] = _T("");
                    GetDlgItemText(hwnd, IDC_GP_PWD, password, 100);
                    
                    if (!_tcscmp(password, _T("")))
                        break;
                    
                    WindowsVector *windows = Windows_Get(hwnd);
                    if (windows->at(index).password != password)
                        Lang_DlgItemText(hwnd, IDC_GP_STATUS, IDS_SGP_ST_WRONGPWD);
                    else
                    {
                        windows->at(index).password = _T("");
                        EndDialog(hwnd, IDOK);
                    }
                }
                break;
            }
        }
        break;
    }
    return FALSE;
}
BOOL CALLBACK SetPasswordDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int index = 0;
    
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            index = lParam-1;
            
            Lang_WindowText(hwnd, IDS_SP_WINDOW);
            Lang_DlgItemText(hwnd, IDH_SP_PWD1, IDS_SGP_PWD1);
            Lang_DlgItemText(hwnd, IDH_SP_PWD2, IDS_SGP_PWD2);
            Lang_DlgItemText(hwnd, IDOK, IDS_DLG_OK);
            Lang_DlgItemText(hwnd, IDCANCEL, IDS_DLG_CANCEL);
        }
        break;
        
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
                
                case IDOK:
                {
                    TCHAR pwd1[100] = _T(""), pwd2[100] = _T("");
                    GetDlgItemText(hwnd, IDC_SP_PWD1, pwd1, 100);
                    GetDlgItemText(hwnd, IDC_SP_PWD2, pwd2, 100);
                    
                    if (!_tcscmp(pwd1, _T("")))
                    {
                        ErrorBox(hwnd, FALSE, IDS_SGP_ST_ENTERPWD);
                        break;
                    }
                    if (_tcslen(pwd1) < 4)
                    {
                        ErrorBox(hwnd, FALSE, IDS_SGP_ST_PWDLENGTH);
                        break;
                    }
                    if (!_tcscmp(pwd2, _T("")))
                    {
                        ErrorBox(hwnd, FALSE, IDS_SGP_ST_REPEATPWD);
                        break;
                    }
                    if (_tcscmp(pwd1, pwd2))
                    {
                        ErrorBox(hwnd, FALSE, IDS_SGP_ST_DIFFER);
                        break;
                    }
                    
                    WindowsVector *windows = Windows_Get(hwnd);
                    windows->at(index).password = pwd1;
                    
                    EndDialog(hwnd, IDOK);
                }
                break;
            }
        }
        break;
        
        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;
    }
    
    return FALSE;
}
