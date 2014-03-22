#include "resource.h"
#include "CommonFuncs.h"
#include "LangFuncs.h"
#include "MainFuncs.h"
#include "OptionsFuncs.h"

#include "WindowsList.h"


BOOL CALLBACK WindowsListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC originalProc = (WNDPROC)GetProp(hwnd, ORIGINALPROCPROP);
    
    switch(msg)
    {
        case WM_TIMER:
            WindowsList_Update(hwnd);
            return 0;
        
        case WM_LANGUAGECHANGED:
            WindowsList_LanguageChanged(hwnd);
            break;
    }
    
    return CallWindowProc(originalProc, hwnd, msg, wParam, lParam);
}

int CALLBACK WindowsList_CompareFunc(LPARAM firstItemIndex, LPARAM secondItemIndex, LPARAM lParam)
{
    HWND windowList = (HWND)lParam;
    int orderIndex = Options_GetInt(windowList, WL_ORDER_INDEX);
    BOOL orderAscending = Options_GetInt(windowList, WL_ORDER_ASCENDING);
    
    TCHAR firstItemText[260], secondItemText[260];
    ListView_GetItemText(windowList, firstItemIndex, orderIndex, firstItemText, 260);
    ListView_GetItemText(windowList, secondItemIndex, orderIndex, secondItemText, 260);
    
    return orderAscending ? 
        _tcscmp( _tcslwr(secondItemText), _tcslwr(firstItemText) ) :
        _tcscmp( _tcslwr(firstItemText), _tcslwr(secondItemText) );
}

void WindowsList_OnColumnClick(NMLISTVIEW *nmlv)
{
    HWND windowsList = nmlv->hdr.hwndFrom;
    
    //The clicked column is still the same?
    BOOL isAscending = Options_GetInt(windowsList, WL_ORDER_ASCENDING);
    if (Options_GetInt(windowsList, WL_ORDER_INDEX) == nmlv->iSubItem)
        isAscending = !isAscending; //Then invert order
    
    //Set options
    Options_SetInt(windowsList, WL_ORDER_INDEX, nmlv->iSubItem);
    Options_SetInt(windowsList, WL_ORDER_ASCENDING, isAscending);
    
    //Display the sort order
    ListView_SetHeaderSortImage(windowsList, nmlv->iSubItem, isAscending);
    
    //Finally, sort
    ListView_SortItemsEx(windowsList, WindowsList_CompareFunc, windowsList);
}
void WindowsList_OnHotTrack(NMLISTVIEW *nmlv)
{
    HWND windowsList = nmlv->hdr.hwndFrom;
    HWND hwndMain = GetMainWindow(windowsList);
    
    if (nmlv->iItem == -1)
    {
        SendDlgItemMessage(hwndMain, IDC_STATUSBAR, SB_SETTEXT, 0,
            (LPARAM)Lang_LoadString(hwndMain, IDS_SB_READY).c_str());
    }
    else
    {
        WindowsVector *windows = Windows_Get(hwndMain);
        
        int uniqueId = ListView_GetItemParam(windowsList, nmlv->iItem);
        int index = FindWindow_FromUniqueId(hwndMain, uniqueId);
        
        tstring program = _T("");
        if (windows->at(index).windowText.size() > 25)
            program = windows->at(index).windowText.substr(0, 25) + _T("...");
        else
            program = windows->at(index).windowText;
        
        tstring statusBarText = _T("");
        if (windows->at(index).isMinimized)
            statusBarText = lstrprintf(hwndMain, IDS_SB_WL_RESTORE, program.c_str());
        else
            statusBarText = lstrprintf(hwndMain, IDS_SB_WL_MINIMIZE, program.c_str());
        
        SendDlgItemMessage(hwndMain, IDC_STATUSBAR, SB_SETTEXT, 0, (LPARAM)statusBarText.c_str());
    }
}
void WindowsList_OnLeftClick(NMITEMACTIVATE *nmia)
{
    if (nmia->iItem == -1)
    {
        HWND windowsList = nmia->hdr.hwndFrom;
        MainDlg_MoveOnClick(windowsList);
    }
}
void WindowsList_OnDoubleClick(NMITEMACTIVATE *nmia)
{
    HWND windowsList = nmia->hdr.hwndFrom;
    HWND hwndMain = GetMainWindow(windowsList);
    
    if (nmia->iItem != -1)
    {
        WindowsVector *windows = Windows_Get(hwndMain);
        
        int uniqueId = ListView_GetItemParam(windowsList, nmia->iItem);
        int index = FindWindow_FromUniqueId(hwndMain, uniqueId);
        
        if(windows->at(index).isMinimized == FALSE)
            Window_Minimize(hwndMain, index);
        else
            Window_Restore(hwndMain, index);
    }
}
void WindowsList_OnRightClick(NMITEMACTIVATE *nmia)
{
    HWND windowsList = nmia->hdr.hwndFrom;
    HWND hwndMain = GetMainWindow(windowsList);
    
    if (nmia->iItem != -1)
    {
        WindowsVector *windows = Windows_Get(hwndMain);
        
        int uniqueId = ListView_GetItemParam(windowsList, nmia->iItem);
        int index = FindWindow_FromUniqueId(hwndMain, uniqueId);
        
        HMENU popupMenu = CreatePopupMenu();
        if(windows->at(index).isMinimized == FALSE)
            Lang_AppendMenu(hwndMain, popupMenu, MF_STRING, IDMC_WL_MENU_MINIMIZE, IDS_WL_MENU_MINIMIZE);
        else
            Lang_AppendMenu(hwndMain, popupMenu, MF_STRING, IDMC_WL_MENU_RESTORE, IDS_WL_MENU_RESTORE);
        
        AppendMenu(popupMenu, MF_SEPARATOR, 0,0);
        
        IntVector *options = Options_GetProgramOptionsVector(hwndMain, windows->at(index).path);
        {
            Lang_AppendMenu(hwndMain, popupMenu, MF_STRING|
                (options->at(PROGRAMOPTION_ALWAYSMINIMIZE) ? MF_CHECKED : MF_UNCHECKED)|
                (options->at(PROGRAMOPTION_NEVERMINIMIZE) ? MF_GRAYED : 0), 
                PROGRAMOPTION_ALWAYSMINIMIZE+10, IDS_OPTS_PL_ALWAYSMINIMIZE);
            
            Lang_AppendMenu(hwndMain, popupMenu, MF_STRING|
                (options->at(PROGRAMOPTION_NEVERMINIMIZE) ? MF_CHECKED : MF_UNCHECKED)|
                (options->at(PROGRAMOPTION_ALWAYSMINIMIZE) ? MF_GRAYED : 0),
                PROGRAMOPTION_NEVERMINIMIZE+10, IDS_OPTS_PL_NEVERMINIMIZE);
            
            Lang_AppendMenu(hwndMain, popupMenu, MF_STRING|
                (options->at(PROGRAMOPTION_NOAUTOMINIMIZE) ? MF_CHECKED : MF_UNCHECKED)|
                (options->at(PROGRAMOPTION_ALWAYSMINIMIZE) ? MF_GRAYED : 0)|
                (options->at(PROGRAMOPTION_NEVERMINIMIZE) ? MF_GRAYED : 0)|
                (Options_GetInt(hwndMain, TRAY_AUTOMIN) == FALSE ? MF_GRAYED : 0),
                PROGRAMOPTION_NOAUTOMINIMIZE+10, IDS_OPTS_PL_NOAUTOMINIMIZE);
            
            Lang_AppendMenu(hwndMain, popupMenu, MF_STRING|
                (options->at(PROGRAMOPTION_NOTRAYBUTTON) ? MF_CHECKED : MF_UNCHECKED)|
                (options->at(PROGRAMOPTION_ALWAYSMINIMIZE) ? MF_GRAYED : 0)|
                (options->at(PROGRAMOPTION_NEVERMINIMIZE) ? MF_GRAYED : 0)|
                (Options_GetInt(hwndMain, TRAY_HOOK) != HO_TRAYBUTTON ? MF_GRAYED : 0),
                PROGRAMOPTION_NOTRAYBUTTON+10, IDS_OPTS_PL_NOTRAYBUTTON);
            
            Lang_AppendMenu(hwndMain, popupMenu, MF_STRING|
                (options->at(PROGRAMOPTION_NOHOOKMINBUTTON) ? MF_CHECKED : MF_UNCHECKED)|
                (options->at(PROGRAMOPTION_ALWAYSMINIMIZE) ? MF_GRAYED : 0)|
                (options->at(PROGRAMOPTION_NEVERMINIMIZE) ? MF_GRAYED : 0)|
                (Options_GetInt(hwndMain, TRAY_HOOK) != HO_MINBUTTON ? MF_GRAYED : 0), 
                PROGRAMOPTION_NOHOOKMINBUTTON+10, IDS_OPTS_PL_NOHOOKMINBUTTON);
        }
        
        AppendMenu(popupMenu, MF_SEPARATOR, 0,0);
        Lang_AppendMenu(hwndMain, popupMenu, MF_STRING, IDMC_WL_MENU_PROPS, IDS_WL_MENU_PROPS);
        
        ClientToScreen(windowsList, &(nmia->ptAction));
        UINT cmd = TrackPopupMenuEx(popupMenu,
            TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON|TPM_RETURNCMD,
            nmia->ptAction.x, nmia->ptAction.y, hwndMain, NULL);
        
        switch(cmd)
        {
            case IDMC_WL_MENU_PROPS:
            {
                SHELLEXECUTEINFO sei = {0};
                sei.cbSize = sizeof(SHELLEXECUTEINFO);
                sei.lpFile = windows->at(index).path.c_str();
                sei.lpVerb = _T("properties");
                sei.fMask  = SEE_MASK_INVOKEIDLIST;
                sei.nShow  = SW_SHOWNORMAL;
                
                ShellExecuteEx(&sei); 
            }
            break;
            
            case IDMC_WL_MENU_MINIMIZE:
                Window_Minimize(hwndMain, index);
                break;
            
            case IDMC_WL_MENU_RESTORE:
                Window_Restore(hwndMain, index);
                break;
            
            case PROGRAMOPTION_ALWAYSMINIMIZE+10:
            case PROGRAMOPTION_NEVERMINIMIZE+10:
            case PROGRAMOPTION_NOAUTOMINIMIZE+10:
            case PROGRAMOPTION_NOTRAYBUTTON+10:
            case PROGRAMOPTION_NOHOOKMINBUTTON+10:
            {
                IntVector *programOptions = Options_GetProgramOptionsVector(hwndMain, windows->at(index).path);
                programOptions->at(cmd-10) = !programOptions->at(cmd-10);
                
                //Notify the hooked window about the option change
                SendMessage(hwndMain, getHookOptionsMsg, (WPARAM)windows->at(index).hwnd, 0);
            }
            break;
        }
    }
}

int WindowsList_FindItem(HWND listView, unsigned long id)
{
    int count = ListView_GetItemCount(listView);
    for (int i = 0; i<count; i++)
    {
        LVITEM li = {0};
        li.iItem = i;
        li.mask = LVIF_PARAM;
        ListView_GetItem(listView, &li);
        
        if (li.lParam == id)
            return i;
    }
    
    return -1;
}
void WindowsList_Init(HWND listView)
{
    //Extended styles
    ListView_SetExtendedListViewStyle(listView, LVS_EX_FULLROWSELECT);
    
    //Subclass the listview
    WNDPROC originalProc = (WNDPROC)SetWindowLong(listView, GWL_WNDPROC, (LONG)WindowsListViewProc);
    SetProp(listView, ORIGINALPROCPROP, (HANDLE)originalProc);
    
    //Imagelist
    HIMAGELIST imageList = ImageList_Create(16,16, ILC_COLOR32|ILC_MASK, 10, 10);
    (void)ListView_SetImageList(listView, imageList, LVSIL_SMALL);
    
    tstring column1 = Lang_LoadString(listView, IDS_WL_WINDOWNAME);
    tstring column2 = Lang_LoadString(listView, IDS_WL_STATUS);
    tstring column3 = Lang_LoadString(listView, IDS_WL_FILENAME);
    
    //Columns
    LVCOLUMN lc = {0};
    lc.mask = LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
    
    lc.cx = 280;
    lc.iSubItem = 0;
    lc.pszText = (LPTSTR)column1.c_str();
    ListView_InsertColumn(listView, 0, &lc);
    
    lc.cx = 60;
    lc.iSubItem = 1;
    lc.pszText = (LPTSTR)column2.c_str();
    ListView_InsertColumn(listView, 1, &lc);
    
    lc.cx = 100;
    lc.iSubItem = 2;
    lc.pszText = (LPTSTR)column3.c_str();
    ListView_InsertColumn(listView, 2, &lc);
    
    SetTimer(listView, 2, 1000, NULL);
}
void WindowsList_Update(HWND windowsList)
{
    WindowsVector *windows = Windows_Get(windowsList);
    HIMAGELIST imageList = ListView_GetImageList(windowsList, LVSIL_SMALL);
    
    BOOL removed = FALSE;
    BOOL added = FALSE;
    for (int i = 0; i<windows->size(); i++)
    {
        //Insert
        if (windows->at(i).toAdd == TRUE)
        {
            windows->at(i).toAdd = FALSE;
            added = TRUE;
            
            HICON progIcon = Window_GetIcon(windowsList, i);
            windows->at(i).imageIndex = ImageList_AddIcon(imageList, progIcon);
            DestroyIcon(progIcon);
            
            //Create an unique id
            windows->at(i).uniqueId = GetUniqueId();
            
            LVITEM li = {0};
            li.mask = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE;
            li.iItem = ListView_GetItemCount(windowsList);
            
            li.iImage = windows->at(i).imageIndex;
            li.lParam = windows->at(i).uniqueId;
            li.iSubItem = 0;
            li.pszText = (LPTSTR)windows->at(i).windowText.c_str();
            int itemIndex = ListView_InsertItem(windowsList, &li);
            
            li.mask = LVIF_TEXT;
            li.iItem = itemIndex;
            
            li.iSubItem = 1;
            li.pszText = (LPTSTR)Lang_LoadString(windowsList, IDS_WL_VISIBLE).c_str();
            ListView_SetItem(windowsList, &li);
            
            li.iSubItem = 2;
            li.pszText = (LPTSTR)windows->at(i).filename.c_str();
            ListView_SetItem(windowsList, &li);
            
            //Insert the program path in the options
            //(if it doesn't exists)
            IntVector *options = Options_GetProgramOptionsVector(windowsList, windows->at(i).path);
            if (options->at(PROGRAMOPTION_ALWAYSMINIMIZE) == TRUE)
                Window_Minimize(windowsList, i);
        }
        //Update or remove (if necessary)
        else
        {
            int lvIndex = WindowsList_FindItem(windowsList, windows->at(i).uniqueId);
            if (lvIndex == -1)
                continue;
            
            //Remove
            if (windows->at(i).toRemove)
            {
                ListView_DeleteItem(windowsList, lvIndex);
                
                HIMAGELIST imageList = ListView_GetImageList(windowsList, LVSIL_SMALL);
                ImageList_Remove(imageList, windows->at(i).imageIndex);
                
                if (windows->at(i).isMinimized)
                    TrayIcon_Remove(windowsList, i);
                
                windows->erase( windows->begin()+i );
                i -= 1;
                removed = TRUE;
                
                continue;
            }
            
            if (windows->at(i).toUpdate & WTU_WINDOWTEXT)
            {
                LVITEM li = {0};
                li.mask = LVIF_TEXT;
                li.iItem = lvIndex;
                li.iSubItem = 0;
                li.pszText = (LPTSTR)windows->at(i).windowText.c_str();
                ListView_SetItem(windowsList, &li);
            }
            
            if (windows->at(i).toUpdate & WTU_VISIBLE)
            {
                tstring visibleText = (windows->at(i).isMinimized ?
                    Lang_LoadString(windowsList, IDS_WL_MINIMIZED) : 
                    Lang_LoadString(windowsList, IDS_WL_VISIBLE));
                
                LVITEM li = {0};
                li.mask = LVIF_TEXT;
                li.iItem = lvIndex;
                li.iSubItem = 1;
                li.pszText = (LPTSTR)visibleText.c_str();
                ListView_SetItem(windowsList, &li);
            }
            
            windows->at(i).toUpdate = WTU_NONE;
        }
    }
    
    //Refresh routines
    if (removed || added)
    {
        //Refresh windows count
        HWND hwndMain = GetMainWindow(windowsList);
        tstring windowsCount = lstrprintf(hwndMain, IDS_SB_WINDOWSCOUNT, windows->size());
        SendDlgItemMessage(hwndMain, IDC_STATUSBAR, SB_SETTEXT, 1, (LPARAM)windowsCount.c_str());
    }
    
    if (added)
    {
        //Resort the listview if it's sorted
        if (Options_GetInt(windowsList, WL_ORDER_INDEX) != -1)
        {
            BOOL isAscending = Options_GetInt(windowsList, WL_ORDER_ASCENDING);
            int orderIndex = Options_GetInt(windowsList, WL_ORDER_INDEX);
            
            //Display the sort order
            ListView_SetHeaderSortImage(windowsList, orderIndex, isAscending);
            
            //Sort
            ListView_SortItemsEx(windowsList, WindowsList_CompareFunc, windowsList);
        }
    }
    
    if (removed)
    {
        ImageList_RemoveAll(imageList);
        
        int count = ListView_GetItemCount(windowsList);
        for (int i = 0; i<count; i++)
        {
            unsigned long uid = ListView_GetItemParam(windowsList, i);
            int index = FindWindow_FromUniqueId(windowsList, uid);
            
            HICON icon = Window_GetIcon(windowsList, index);
            windows->at(index).imageIndex = ImageList_AddIcon(imageList, icon);
            DestroyIcon(icon);
            
            LVITEM li = {0};
            li.mask = LVIF_IMAGE;
            li.iItem = i;
            li.iImage = windows->at(index).imageIndex;
            ListView_SetItem(windowsList, &li);
        }
    }
}
void WindowsList_LanguageChanged(HWND listView)
{
    LVCOLUMN lc = {0};
    lc.mask = LVCF_TEXT;
    
    lc.pszText = (LPTSTR)Lang_LoadString(listView, IDS_WL_WINDOWNAME).c_str();
    ListView_SetColumn(listView, 0, &lc);
    
    lc.pszText = (LPTSTR)Lang_LoadString(listView, IDS_WL_STATUS).c_str();
    ListView_SetColumn(listView, 1, &lc);
    
    lc.pszText = (LPTSTR)Lang_LoadString(listView, IDS_WL_FILENAME).c_str();
    ListView_SetColumn(listView, 2, &lc);
    
    
    WindowsVector *windows = Windows_Get(listView);
    
    int count = ListView_GetItemCount(listView);
    for (int i = 0; i<count; i++)
    {
        if (windows->at(i).isMinimized == FALSE)
        {
            ListView_SetItemText(listView, i, 1, (LPTSTR)
                Lang_LoadString(listView, IDS_WL_VISIBLE).c_str());
        }
        else
        {
            ListView_SetItemText(listView, i, 1, (LPTSTR)
                Lang_LoadString(listView, IDS_WL_MINIMIZED).c_str());
        }
    }
}
