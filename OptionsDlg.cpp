#include "resource.h"
#include "CommonFuncs.h"
#include "HookFuncs.h"
#include "LangFuncs.h"
#include "MainFuncs.h"
#include "OptionsFuncs.h"
#include "StringConversions.h"

#define OPTSMODIFIEDPROP        _T("OptionsModified")
#define OPTSDIALOGSPROP         _T("OptionsDialogs")
#define TOOLTIPPROP             _T("ToolTip")

#define ITEM_COLLAPSED              0
#define ITEM_EXPANDED               1

#define IMAGE_CHECKBOXUNCHECKED     0
#define IMAGE_CHECKBOXCHECKED       1
#define IMAGE_CHECKBOXDISABLED      2
#define IMAGE_CHECKBOXDISABLEDCHECKED 3
#define IMAGE_CHECKBOXCHECKEDHOT    4
#define IMAGE_CHECKBOXUNCHECKEDHOT  5
#define IMAGE_COLLAPSED             6
#define IMAGE_EXPANDED              7

typedef std::map<int,HWND> OptionsDialogs;
typedef HRESULT (WINAPI *PFNCLOSETHEMEDATA)(HTHEME);
typedef HRESULT (WINAPI *PFNDRAWTHEMEBACKGROUND)(HTHEME,HDC,int,int,const LPRECT,const LPRECT);
typedef HTHEME (WINAPI *PFNOPENTHEMEDATA)(HWND,LPCWSTR);

typedef struct tagPROGRAMSLISTSTRUCT
{
    tagPROGRAMSLISTSTRUCT(BOOL psubitem, int pparentItem, int pprogramOption)
    {
        Init();
        subitem = psubitem;
        parentItem = pparentItem;
        programOption = pprogramOption;
    }
    tagPROGRAMSLISTSTRUCT(int pstate, tstring ppath, BOOL psubitem, IntVector& poptions)
    {
        Init();
        state = pstate;
        path = ppath;
        subitem = psubitem;
        options = poptions;
    }
    tagPROGRAMSLISTSTRUCT(tstring ppath)
    {
        Init();
        path = ppath;
    }
    
    void Init()
    {
        state = 0;
        path = _T("");
        subitem = FALSE;
        parentItem = 0;
        programOption = 0;
        stayDisabled = FALSE;
    }
    
    //Item
    int state;
    tstring path;
    IntVector options;
    
    //Subitem
    BOOL subitem;
    int parentItem;
    int programOption;
    BOOL stayDisabled;
} PROGRAMSLISTSTRUCT;

void GeneralOptionsDlg_Restore(HWND hwnd)
{
    CheckDlgButton(hwnd, IDC_GOD_WINDOWS_RUN, Options_GetInt(hwnd, STARTUP));
    
    int checkUpdates = Options_GetInt(hwnd, UPDATES_CHECK);
    if (checkUpdates == UG_NONE)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_GOD_UPD_EVERYSTART), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_GOD_UPD_EVERYDAY), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_GOD_UPD_EVERYWEEK), FALSE);
    }
    else
    {
        UINT itemId = 0;
        switch(checkUpdates)
        {
            case UG_EVERYSTART: itemId = IDC_GOD_UPD_EVERYSTART; break;
            case UG_EVERYDAY:   itemId = IDC_GOD_UPD_EVERYDAY; break;
            case UG_EVERYWEEK:  itemId = IDC_GOD_UPD_EVERYWEEK; break;
        }
        
        CheckDlgButton(hwnd, IDC_GOD_UPD_CHECK, TRUE);
        CheckRadioButton(hwnd, IDC_GOD_UPD_EVERYSTART, IDC_GOD_UPD_EVERYWEEK, itemId);
    }
    
    tstring language = Options_GetString(hwnd, LANGUAGE);
    {
        HWND languageCB = GetDlgItem(hwnd, IDC_GOD_LANGUAGE);
        int languageCount = ComboBox_GetCount(languageCB);
        for (int i = 0; i<languageCount; i++)
        {
            TCHAR *itemData = (LPTSTR)ComboBox_GetItemData(languageCB, i);
            if (itemData && itemData == language)
            {
                ComboBox_SetCurSel(languageCB, i);
                break;
            }
        }
    }
}
void TrayOptionsDlg_Restore(HWND hwnd)
{
    CheckDlgButton(hwnd, IDC_TOD_TE_MINONCLOSE, Options_GetInt(hwnd, MAINTRAY_MINONCLOSE));
    CheckDlgButton(hwnd, IDC_TOD_TE_HIDEONMIN, Options_GetInt(hwnd, MAINTRAY_HIDEONMIN));
    CheckDlgButton(hwnd, IDC_TOD_OP_NOICON, Options_GetInt(hwnd, TRAY_NOICON));
    CheckDlgButton(hwnd, IDC_TOD_OP_PWDPROTECT, Options_GetInt(hwnd, TRAY_PWDPROTECT));
    CheckDlgButton(hwnd, IDC_TOD_OP_GROUPICON, Options_GetInt(hwnd, TRAY_GROUPICON));
}
void Tray2OptionsDlg_Restore(HWND hwnd)
{
    CheckDlgButton(hwnd, IDC_T2OD_AUTOMINIMIZE, Options_GetInt(hwnd, TRAY_AUTOMIN));
    SetDlgItemInt(hwnd, IDC_T2OD_AM_TIME, Options_GetInt(hwnd, TRAY_AUTOMIN_TIME), TRUE);
    
    HWND timeType = GetDlgItem(hwnd, IDC_T2OD_AM_TIMETYPE);
    switch(Options_GetInt(hwnd, TRAY_AUTOMIN_TIMETYPE))
    {
        default:
        case TT_SECONDS:
            ComboBox_SetCurSel(timeType, 0);
            break;
            
        case TT_MINUTES:
            ComboBox_SetCurSel(timeType, 1);
            break;
    }
    
    UINT hookOptionId = 0;
    switch( Options_GetInt(hwnd, TRAY_HOOK) )
    {
        case HO_NONE:           hookOptionId = IDC_T2OD_MFP_NONE; break;
        case HO_TRAYBUTTON:     hookOptionId = IDC_T2OD_MFP_TRAYBUTTON; break;
        case HO_MINBUTTON:      hookOptionId = IDC_T2OD_MFP_MINBUTTON; break;
        case HO_MIDDLECLICK:    hookOptionId = IDC_T2OD_MFP_MIDDLECLICK; break;
    }
    CheckRadioButton(hwnd, IDC_T2OD_MFP_NONE, IDC_T2OD_MFP_MIDDLECLICK, hookOptionId);
}
void ProgramsOptionsDlg_Restore(HWND hwnd)
{
    
}
void HotkeysOptionsDlg_Restore(HWND hwnd)
{
    int kbHotkey = Options_GetInt(hwnd, KBHOTKEY);
    if (kbHotkey == KBH_NONE)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_HOD_KBH_F7), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_HOD_KBH_WINQ), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_HOD_KBH_WINS), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_HOD_KBH_CTRLQ), FALSE);
    }
    else
    {
        CheckDlgButton(hwnd, IDC_HOD_KBH_ENABLE, BST_CHECKED);
        
        UINT itemId = 0;
        switch(kbHotkey)
        {
            case KBH_F7:    itemId = IDC_HOD_KBH_F7; break;
            case KBH_WINQ:  itemId = IDC_HOD_KBH_WINQ; break;
            case KBH_WINS:  itemId = IDC_HOD_KBH_WINS; break;
            case KBH_CTRLQ: itemId = IDC_HOD_KBH_CTRLQ; break;
        }
        
        CheckRadioButton(hwnd, IDC_HOD_KBH_F7, IDC_HOD_KBH_CTRLQ, itemId);
    }
}

void GeneralOptionsDlg_Save(HWND hwnd)
{
    if (IsDlgButtonChecked(hwnd, IDC_GOD_WINDOWS_RUN))
    {
        Options_SetInt(hwnd, STARTUP, TRUE);
        
        //Find the executable path
        tstring fullPath = GetProgramFullPath();
        
        //Append the option
        fullPath = _T('"') + fullPath + _T('"');
        fullPath += _T(" /windowsstartup");
        
        //Set the registry key
        HKEY regKey;
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0,
            KEY_WRITE|KEY_QUERY_VALUE, &regKey);
        if (RegQueryValueEx(regKey, _T("TrayEverything"), NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
        {
            RegSetValueEx(regKey, _T("TrayEverything"), 0, REG_SZ, (CONST BYTE*)fullPath.c_str(), 
                fullPath.size() * sizeof(TCHAR));
        }
        RegCloseKey(regKey);
    }
    else
    {
        Options_SetInt(hwnd, STARTUP, FALSE);
        
        HKEY regKey;
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0,
            KEY_WRITE|KEY_QUERY_VALUE, &regKey);
        if (RegQueryValueEx(regKey, _T("TrayEverything"), NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            RegDeleteValue(regKey, _T("TrayEverything"));
        RegCloseKey(regKey);
    }
    
    if(!IsDlgButtonChecked(hwnd, IDC_GOD_UPD_CHECK))
        Options_SetInt(hwnd, UPDATES_CHECK, UG_NONE);
    else
    {
        if (IsDlgButtonChecked(hwnd, IDC_GOD_UPD_EVERYSTART))
            Options_SetInt(hwnd, UPDATES_CHECK, UG_EVERYSTART);
        else if (IsDlgButtonChecked(hwnd, IDC_GOD_UPD_EVERYDAY))
            Options_SetInt(hwnd, UPDATES_CHECK, UG_EVERYDAY);
        else if (IsDlgButtonChecked(hwnd, IDC_GOD_UPD_EVERYWEEK))
            Options_SetInt(hwnd, UPDATES_CHECK, UG_EVERYWEEK);
    }
    
    HWND languageCB = GetDlgItem(hwnd, IDC_GOD_LANGUAGE);
    int curSel = ComboBox_GetCurSel(languageCB);
    TCHAR *newLanguage = (LPTSTR)ComboBox_GetItemData(languageCB, curSel);
    tstring oldLanguage = Options_GetString(hwnd, LANGUAGE);
    if (newLanguage && newLanguage != oldLanguage)
    {
        Options_SetString(hwnd, LANGUAGE, newLanguage);
        
        Lang_Unload(hwnd);
        Lang_Load(hwnd);
        
        HWND hwndMain = GetMainWindow(hwnd);
        SendMessage(hwndMain, WM_LANGUAGECHANGED, 0,0);
    }
}
void TrayOptionsDlg_Save(HWND hwnd)
{
    Options_SetInt(hwnd, MAINTRAY_MINONCLOSE, IsDlgButtonChecked(hwnd, IDC_TOD_TE_MINONCLOSE));
    Options_SetInt(hwnd, MAINTRAY_HIDEONMIN, IsDlgButtonChecked(hwnd, IDC_TOD_TE_HIDEONMIN));
    
    Options_SetInt(hwnd, TRAY_NOICON, IsDlgButtonChecked(hwnd, IDC_TOD_OP_NOICON));
    Options_SetInt(hwnd, TRAY_PWDPROTECT, IsDlgButtonChecked(hwnd, IDC_TOD_OP_PWDPROTECT));
    
    BOOL oldOption = Options_GetInt(hwnd, TRAY_GROUPICON);
    BOOL newOption = IsDlgButtonChecked(hwnd, IDC_TOD_OP_GROUPICON);
    if (oldOption != newOption)
    {
        TrayIcon_RemoveAll(hwnd, FALSE);
        Options_SetInt(hwnd, TRAY_GROUPICON, newOption);
        
        WindowsVector *windows = Windows_Get(hwnd);
        if (newOption == TRUE)
        {
            std::map<tstring, bool> icons;
            unsigned long uniqueGroupId = 0;
            for (int i = 0; i<windows->size(); i++)
            {
                if (windows->at(i).isMinimized)
                {
                    if (icons.find(windows->at(i).path) == icons.end())
                    {
                        uniqueGroupId = GetUniqueId();
                        windows->at(i).uniqueGroupId = uniqueGroupId;
                        
                        TrayIcon_Set(hwnd, i);
                    }
                    else
                    {
                        windows->at(i).uniqueGroupId = uniqueGroupId;
                        TrayIcon_CreateMenu(hwnd, i);
                    }
                    
                    icons[ windows->at(i).path ] = true;
                }
            }
        }
        else
        {
            for (int i = 0; i<windows->size(); i++)
            {
                if (windows->at(i).isMinimized)
                    TrayIcon_Set(hwnd, i);
            }
        }
    }
    
    BOOL oldHookOpt = Options_GetInt(hwnd, TRAY_HOOK) >= 0;
    BOOL newHookOpt = !IsDlgButtonChecked(hwnd, IDC_T2OD_MFP_NONE);
    if (oldHookOpt != newHookOpt)
    {
        if (IsDlgButtonChecked(hwnd, IDC_T2OD_MFP_NONE))
            Options_SetInt(hwnd, TRAY_HOOK, HO_NONE);
        else if (IsDlgButtonChecked(hwnd, IDC_T2OD_MFP_TRAYBUTTON))
            Options_SetInt(hwnd, TRAY_HOOK, HO_TRAYBUTTON);
        else if (IsDlgButtonChecked(hwnd, IDC_T2OD_MFP_MINBUTTON))
            Options_SetInt(hwnd, TRAY_HOOK, HO_MINBUTTON);
        
        if (Options_GetInt(hwnd, TRAY_HOOK) != HO_NONE)
            Hook_Start();
        else
            Hook_Stop();
    }
}
void Tray2OptionsDlg_Save(HWND hwnd)
{
    Options_SetInt(hwnd, TRAY_AUTOMIN, IsDlgButtonChecked(hwnd, IDC_T2OD_AUTOMINIMIZE));
    Options_SetInt(hwnd, TRAY_AUTOMIN_TIME, GetDlgItemInt(hwnd, IDC_T2OD_AM_TIME, NULL, TRUE));
    
    HWND timeType = GetDlgItem(hwnd, IDC_T2OD_AM_TIMETYPE);
    Options_SetInt(hwnd, TRAY_AUTOMIN_TIMETYPE, ComboBox_GetCurSel(timeType));
    
    if (IsDlgButtonChecked(hwnd, IDC_T2OD_MFP_NONE))
        Options_SetInt(hwnd, TRAY_HOOK, HO_NONE);
    else if (IsDlgButtonChecked(hwnd, IDC_T2OD_MFP_TRAYBUTTON))
        Options_SetInt(hwnd, TRAY_HOOK, HO_TRAYBUTTON);
    else if (IsDlgButtonChecked(hwnd, IDC_T2OD_MFP_MINBUTTON))
        Options_SetInt(hwnd, TRAY_HOOK, HO_MINBUTTON);
    else if (IsDlgButtonChecked(hwnd, IDC_T2OD_MFP_MIDDLECLICK))
        Options_SetInt(hwnd, TRAY_HOOK, HO_MIDDLECLICK);
    
    if (Options_GetInt(hwnd, TRAY_HOOK) != HO_NONE)
        Hook_Start();
    else
        Hook_Stop();
    
    //Reset inactive seconds
    WindowsVector *windows = Windows_Get(hwnd);
    for (int i = 0; i<windows->size(); i++)
        windows->at(i).secondsInactive = 0;
}
void ProgramsOptionsDlg_Save(HWND hwnd)
{
    HWND programsListView = GetDlgItem(hwnd, IDC_POD_PROGRAMSLIST);
    int itemsCount = ListView_GetItemCount(programsListView);
    for (int i = 0; i < itemsCount; i++)
    {
        PROGRAMSLISTSTRUCT *pls = (PROGRAMSLISTSTRUCT*)ListView_GetItemParam(programsListView, i);
        if (pls && pls->subitem == FALSE)
            Options_SetProgramOptionsVector(hwnd, pls->path, &pls->options);
    }
    
    SendMessage(HWND_BROADCAST, updateHookOptionsMsg, 0,0);
}
void HotkeysOptionsDlg_Save(HWND hwnd)
{
    //Unregister all the hotkeys
    HWND hwndMain = GetMainWindow(hwnd);
    UnregisterHotKey(hwndMain, MINHOTKEY_ID);
    
    if (IsDlgButtonChecked(hwnd, IDC_HOD_KBH_ENABLE) == BST_UNCHECKED)
        Options_SetInt(hwnd, KBHOTKEY, KBH_NONE);
    else
    {
        if (IsDlgButtonChecked(hwnd, IDC_HOD_KBH_F7))
        {
            RegisterHotKey(hwndMain, MINHOTKEY_ID, 0, VK_F7);
            Options_SetInt(hwnd, KBHOTKEY, KBH_F7);
        }
        else if (IsDlgButtonChecked(hwnd, IDC_HOD_KBH_WINS))
        {
            RegisterHotKey(hwndMain, MINHOTKEY_ID, MOD_WIN, 'S');
            Options_SetInt(hwnd, KBHOTKEY, KBH_WINS);
        }
        else if (IsDlgButtonChecked(hwnd, IDC_HOD_KBH_WINQ))
        {
            RegisterHotKey(hwndMain, MINHOTKEY_ID, MOD_WIN, 'Q');
            Options_SetInt(hwnd, KBHOTKEY, KBH_WINQ);
        }
        else if (IsDlgButtonChecked(hwnd, IDC_HOD_KBH_CTRLQ))
        {
            RegisterHotKey(hwndMain, MINHOTKEY_ID, MOD_CONTROL, 'Q');
            Options_SetInt(hwnd, KBHOTKEY, KBH_CTRLQ);
        }
    }
}

void GeneralOptionsDlg_Update(HWND hwnd)
{
    BOOL checkForUpdates = IsDlgButtonChecked(hwnd, IDC_GOD_UPD_CHECK) == BST_CHECKED;
    EnableWindow(GetDlgItem(hwnd, IDC_GOD_UPD_EVERYSTART), checkForUpdates);
    EnableWindow(GetDlgItem(hwnd, IDC_GOD_UPD_EVERYDAY), checkForUpdates);
    EnableWindow(GetDlgItem(hwnd, IDC_GOD_UPD_EVERYWEEK), checkForUpdates);
}
void TrayOptionsDlg_Update(HWND hwnd)
{
    EnableDlgItem(hwnd, IDH_TOD_TE_HIDEONMIN, IsDlgButtonChecked(hwnd, IDC_TOD_TE_HIDEONMIN));
    EnableDlgItem(hwnd, IDC_TOD_OP_GROUPICON, !IsDlgButtonChecked(hwnd, IDC_TOD_OP_NOICON));
}
void Tray2OptionsDlg_Update(HWND hwnd)
{
    BOOL enabled = IsDlgButtonChecked(hwnd, IDC_T2OD_AUTOMINIMIZE);
    EnableDlgItem(hwnd, IDC_T2OD_AM_TIME, enabled);
    EnableDlgItem(hwnd, IDC_T2OD_AM_TIMETYPE, enabled);
    
    //Disable hook button option for vista
    {
        OSVERSIONINFO ovi = {0};
        ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&ovi);
        
        if (ovi.dwMajorVersion >= 6)
            EnableDlgItem(hwnd, IDC_T2OD_MFP_TRAYBUTTON, FALSE);
    }
}
void ProgramsOptionsDlg_Update(HWND hwnd)
{
    
}
void HotkeysOptionsDlg_Update(HWND hwnd)
{
    BOOL isKbHotkeys = IsDlgButtonChecked(hwnd, IDC_HOD_KBH_ENABLE) == BST_CHECKED;
    EnableWindow(GetDlgItem(hwnd, IDC_HOD_KBH_F7), isKbHotkeys);
    EnableWindow(GetDlgItem(hwnd, IDC_HOD_KBH_WINQ), isKbHotkeys);
    EnableWindow(GetDlgItem(hwnd, IDC_HOD_KBH_WINS), isKbHotkeys);
    EnableWindow(GetDlgItem(hwnd, IDC_HOD_KBH_CTRLQ), isKbHotkeys);
}


void TabDlg_Init(HWND tab, HWND tabdlg)
{
    RECT tabRect;
    GetWindowRect(tabdlg, &tabRect);
    POINT tabDlgPos = {tabRect.left, tabRect.top};
    ScreenToClient(GetParent(tabdlg), &tabDlgPos);
    
    GetWindowRect(tab, &tabRect);
    POINT newTabDlgPos = {tabDlgPos.x + tabRect.left, tabDlgPos.y + tabRect.top};
    ScreenToClient(GetParent(tabdlg), &newTabDlgPos);
    SetWindowPos(tabdlg, NULL, newTabDlgPos.x, newTabDlgPos.y, 0, 0, SWP_NOSIZE);
}
void TabCtrl_Init(HWND tab)
{
    HWND parent = GetParent(tab);
    OptionsDialogs *dlgs = (OptionsDialogs*)GetProp(parent, OPTSDIALOGSPROP);
    
    TCITEM ti = {0};
    ti.mask = TCIF_TEXT|TCIF_PARAM;
    
    ti.lParam = (LPARAM)(*dlgs)[IDD_OPTS_GENDLG];
    ti.pszText = (LPTSTR)Lang_LoadString(tab, IDS_OPT_TAB_GEN).c_str();
    TabCtrl_InsertItem(tab, 0, &ti);
    
    ti.lParam = (LPARAM)(*dlgs)[IDD_OPTS_TRAYDLG];
    ti.pszText = (LPTSTR)Lang_LoadString(tab, IDS_OPT_TAB_TRAY).c_str();
    TabCtrl_InsertItem(tab, 1, &ti);
    
    ti.lParam = (LPARAM)(*dlgs)[IDD_OPTS_TRAY2DLG];
    ti.pszText = (LPTSTR)Lang_LoadString(tab, IDS_OPT_TAB_TRAY2).c_str();
    TabCtrl_InsertItem(tab, 2, &ti);
    
    ti.lParam = (LPARAM)(*dlgs)[IDD_OPTS_PROGRAMSDLG];
    ti.pszText = (LPTSTR)Lang_LoadString(tab, IDS_OPT_TAB_PROGRAMS).c_str();
    TabCtrl_InsertItem(tab, 3, &ti);
    
    ti.lParam = (LPARAM)(*dlgs)[IDD_OPTS_HOTKEYSDLG];
    ti.pszText = (LPTSTR)Lang_LoadString(tab, IDS_OPT_TAB_HK).c_str();
    TabCtrl_InsertItem(tab, 4, &ti);
}
void LanguageCombobox_Refresh(HWND langCombo)
{
    TCHAR filePath[1024] = _T("");
    HANDLE fileHandle = NULL;
    while( GetNextLanguageDll(fileHandle, filePath) != FALSE )
    {
        HMODULE langDll = LoadLibrary(filePath);
        if (langDll)
        {
            //Insert an item with the name of the language as item label
            TCHAR langName[50];
            LoadString(langDll, IDS_NAME, langName, 50);
            int itemIndex = ComboBox_InsertString(langCombo, -1, langName);
            
            //And set its lParam to the short name of the language
            TCHAR *shortLangName = new TCHAR[20];
            _tcsncpy(shortLangName, _T(""), 20); //Zero-ed
            LoadString(langDll, IDS_SHORTNAME, shortLangName, 20);
            ComboBox_SetItemData(langCombo, itemIndex, shortLangName);
            
            FreeLibrary(langDll);
        }
    }
}
void LanguageCombobox_Free(HWND langCombo)
{
    int itemsCount = ComboBox_GetCount(langCombo);
    for (int i = 0; i<itemsCount; i++)
    {
        TCHAR *shortLangName = (TCHAR*)ComboBox_GetItemData(langCombo, i);
        delete [] shortLangName;
    }
}

BOOL ProgramsListView_IsCheckboxChecked(HWND programsListView, int item)
{
    LVITEM li = {0};
    li.mask = LVIF_IMAGE;
    li.iItem = item;
    ListView_GetItem(programsListView, &li);
    
    return (li.iImage == IMAGE_CHECKBOXCHECKED || 
        li.iImage == IMAGE_CHECKBOXCHECKEDHOT ||
        li.iImage == IMAGE_CHECKBOXDISABLEDCHECKED);
}
BOOL ProgramsListView_IsCheckboxDisabled(HWND programsListView, int item)
{
    LVITEM li = {0};
    li.mask = LVIF_IMAGE;
    li.iItem = item;
    ListView_GetItem(programsListView, &li);
    
    return (li.iImage == IMAGE_CHECKBOXDISABLED ||
        li.iImage == IMAGE_CHECKBOXDISABLEDCHECKED);
}
void ProgramsListView_CheckCheckbox(HWND programsListView, int item)
{
    int image = ListView_GetItemImage(programsListView, item, 0);
    if (image == IMAGE_CHECKBOXUNCHECKEDHOT)
        image = IMAGE_CHECKBOXCHECKEDHOT;
    else if (image == IMAGE_CHECKBOXUNCHECKED)
        image = IMAGE_CHECKBOXCHECKED;
    ListView_SetItemImage(programsListView, item, 0, image);
}
void ProgramsListView_UncheckCheckbox(HWND programsListView, int item)
{
    int image = ListView_GetItemImage(programsListView, item, 0);
    if (image == IMAGE_CHECKBOXCHECKEDHOT)
        image = IMAGE_CHECKBOXUNCHECKEDHOT;
    else if (image == IMAGE_CHECKBOXCHECKED)
        image = IMAGE_CHECKBOXUNCHECKED;
    ListView_SetItemImage(programsListView, item, 0, image);
}
void ProgramsListView_DisableCheckbox(HWND programsListView, int item)
{
    int image = ListView_GetItemImage(programsListView, item, 0);
    if (image == IMAGE_CHECKBOXUNCHECKEDHOT || image == IMAGE_CHECKBOXUNCHECKED)
        image = IMAGE_CHECKBOXDISABLED;
    else if (image == IMAGE_CHECKBOXCHECKEDHOT || image == IMAGE_CHECKBOXCHECKED)
        image = IMAGE_CHECKBOXDISABLEDCHECKED;
    ListView_SetItemImage(programsListView, item, 0, image);
}
void ProgramsListView_EnableCheckbox(HWND programsListView, int item)
{
    int image = ListView_GetItemImage(programsListView, item, 0);
    if (image == IMAGE_CHECKBOXDISABLED)
        image = IMAGE_CHECKBOXUNCHECKED;
    else if (image == IMAGE_CHECKBOXDISABLEDCHECKED)
        image = IMAGE_CHECKBOXCHECKED;
    ListView_SetItemImage(programsListView, item, 0, image);
}
void ProgramsListView_CreateCheckboxUnthemed(HIMAGELIST imageList, HDC hdc, int state)
{
    RECT bitmapRect = {0,0,16,16};
    
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, 16,16);
    HBITMAP oldBitmap = SelectBitmap(memDC, memBitmap);
    DrawFrameControl(memDC, &bitmapRect, DFC_BUTTON, DFCS_BUTTONCHECK|state);
    SelectBitmap(memDC, oldBitmap);
    DeleteDC(memDC);
    
    ImageList_Add(imageList, memBitmap, NULL);
}
void ProgramsListView_CreateCheckboxThemed(HIMAGELIST imageList, HDC hdc, 
    HTHEME theme, PFNDRAWTHEMEBACKGROUND pDrawThemeBackground, int state)
{
    RECT bitmapRect = {1,1,17,17};
    
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, 16,16);
    HBITMAP oldBitmap = SelectBitmap(memDC, memBitmap);
    pDrawThemeBackground(theme, memDC, BP_CHECKBOX, state, &bitmapRect, NULL);
    SelectBitmap(memDC, oldBitmap);
    DeleteDC(memDC);
    
    ImageList_Add(imageList, memBitmap, NULL);
}
void ProgramsListView_FillImageList(HWND programsListView)
{
    HIMAGELIST imageList = ImageList_Create(16,16, ILC_COLOR32, 10,10);
    (void)ListView_SetImageList(programsListView, imageList, LVSIL_SMALL);
    
    HDC hdc = GetDC(NULL);
    BOOL fallBack = TRUE;
    
    if (IsCommCtrlVersion6())
    {
        HMODULE themesDll = LoadLibrary(_T("UXTHEME.DLL"));
        if (themesDll)
        {
            PFNOPENTHEMEDATA pOpenThemeData = (PFNOPENTHEMEDATA)GetProcAddress(themesDll, "OpenThemeData");
            PFNDRAWTHEMEBACKGROUND pDrawThemeBackground = (PFNDRAWTHEMEBACKGROUND)GetProcAddress(themesDll, "DrawThemeBackground");
            PFNCLOSETHEMEDATA pCloseThemeData = (PFNCLOSETHEMEDATA)GetProcAddress(themesDll, "CloseThemeData");
            
            if (pOpenThemeData && pDrawThemeBackground && pCloseThemeData)
            {
                HTHEME theme = pOpenThemeData(programsListView, L"Button");
                
                ProgramsListView_CreateCheckboxThemed(imageList, hdc, theme, pDrawThemeBackground, CBS_UNCHECKEDNORMAL);
                ProgramsListView_CreateCheckboxThemed(imageList, hdc, theme, pDrawThemeBackground, CBS_CHECKEDNORMAL);
                ProgramsListView_CreateCheckboxThemed(imageList, hdc, theme, pDrawThemeBackground, CBS_UNCHECKEDDISABLED);
                ProgramsListView_CreateCheckboxThemed(imageList, hdc, theme, pDrawThemeBackground, CBS_CHECKEDDISABLED);
                ProgramsListView_CreateCheckboxThemed(imageList, hdc, theme, pDrawThemeBackground, CBS_CHECKEDHOT);
                ProgramsListView_CreateCheckboxThemed(imageList, hdc, theme, pDrawThemeBackground, CBS_UNCHECKEDHOT);
                
                pCloseThemeData(theme);
                fallBack = FALSE;
            }
            
            FreeLibrary(themesDll);
        }
    }
    
    //Fall back to windows not-themed checkbox if unable
    //to load themed checkbox functions
    if (fallBack)
    {
        ProgramsListView_CreateCheckboxUnthemed(imageList, hdc, DFCS_CHECKED);
        ProgramsListView_CreateCheckboxUnthemed(imageList, hdc, 0);
        ProgramsListView_CreateCheckboxUnthemed(imageList, hdc, DFCS_INACTIVE);
        ProgramsListView_CreateCheckboxUnthemed(imageList, hdc, DFCS_INACTIVE|DFCS_CHECKED);
        ProgramsListView_CreateCheckboxUnthemed(imageList, hdc, DFCS_CHECKED);
        ProgramsListView_CreateCheckboxUnthemed(imageList, hdc, 0);
    }
    
    ImageList_AddIcon(imageList, LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_EXPAND)));
    ImageList_AddIcon(imageList, LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_COLLAPSE)));
    
    ReleaseDC(NULL, hdc);
}
void ProgramsListView_UpdateSubitems(HWND programsListView, int parentItem, int clickedItem = 0)
{
    for (int subitem = parentItem+1; subitem <= parentItem+5; subitem++)
    {
        if (subitem >= ListView_GetItemCount(programsListView))
            continue;
        
        PROGRAMSLISTSTRUCT *pls = (PROGRAMSLISTSTRUCT*)ListView_GetItemParam(programsListView, subitem);
        if (!pls->subitem)
            break;
        else
        {
            BOOL checked = ProgramsListView_IsCheckboxChecked(programsListView, subitem);
            if (pls->programOption == PROGRAMOPTION_ALWAYSMINIMIZE)
            {
                for (int i = parentItem+1; i <= parentItem+5; i++)
                {
                    if (i == subitem)
                        continue;
                    
                    PROGRAMSLISTSTRUCT *plsSubitem = (PROGRAMSLISTSTRUCT*)ListView_GetItemParam(programsListView, i);
                    if (plsSubitem->stayDisabled)
                        continue;
                    
                    if (checked)
                        ProgramsListView_DisableCheckbox(programsListView, i);
                    else if (clickedItem == subitem)
                        ProgramsListView_EnableCheckbox(programsListView, i);
                }
            }
            else if (pls->programOption == PROGRAMOPTION_NEVERMINIMIZE)
            {
                for (int i = parentItem+1; i <= parentItem+5; i++)
                {
                    if (i == subitem)
                        continue;
                    
                    PROGRAMSLISTSTRUCT *plsSubitem = (PROGRAMSLISTSTRUCT*)ListView_GetItemParam(programsListView, i);
                    if (plsSubitem->stayDisabled)
                        continue;
                    
                    if (checked)
                        ProgramsListView_DisableCheckbox(programsListView, i);
                    else if (clickedItem == subitem)
                        ProgramsListView_EnableCheckbox(programsListView, i);
                }
            }
            else if (pls->programOption == PROGRAMOPTION_NOAUTOMINIMIZE)
            {
                if (Options_GetInt(programsListView, TRAY_AUTOMIN))
                    pls->stayDisabled = FALSE;
                else
                {
                    pls->stayDisabled = TRUE;
                    ProgramsListView_DisableCheckbox(programsListView, subitem);
                }
            }
            else if (pls->programOption == PROGRAMOPTION_NOTRAYBUTTON)
            {
                if (Options_GetInt(programsListView, TRAY_HOOK) == HO_TRAYBUTTON)
                    pls->stayDisabled = FALSE;
                else
                {
                    pls->stayDisabled = TRUE;
                    ProgramsListView_DisableCheckbox(programsListView, subitem);
                }
            }
            else if (pls->programOption == PROGRAMOPTION_NOHOOKMINBUTTON)
            {
                if (Options_GetInt(programsListView, TRAY_HOOK) == HO_MINBUTTON)
                    pls->stayDisabled = FALSE;
                else
                {
                    pls->stayDisabled = TRUE;
                    ProgramsListView_DisableCheckbox(programsListView, subitem);
                }
            }
        }
    }
}
void ProgramsListView_InsertSubitems(HWND programsListView, IntVector& programOptions, int item)
{
    LVITEM li = {0};
    li.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_INDENT|LVIF_PARAM;
    
    li.iItem = item+1;
    li.iIndent = 1;
    li.lParam = (LPARAM)new PROGRAMSLISTSTRUCT(TRUE, item, PROGRAMOPTION_ALWAYSMINIMIZE);
    li.pszText = (LPTSTR)Lang_LoadString(programsListView, IDS_OPTS_PL_ALWAYSMINIMIZE).c_str();
    li.iImage = IMAGE_CHECKBOXUNCHECKED + programOptions.at(PROGRAMOPTION_ALWAYSMINIMIZE);
    ListView_InsertItem(programsListView, &li);
    
    li.iItem = item+2;
    li.iIndent = 1;
    li.lParam = (LPARAM)new PROGRAMSLISTSTRUCT(TRUE, item, PROGRAMOPTION_NEVERMINIMIZE);
    li.pszText = (LPTSTR)Lang_LoadString(programsListView, IDS_OPTS_PL_NEVERMINIMIZE).c_str();
    li.iImage = IMAGE_CHECKBOXUNCHECKED + programOptions.at(PROGRAMOPTION_NEVERMINIMIZE);
    ListView_InsertItem(programsListView, &li);
    
    li.iItem = item+3;
    li.iIndent = 1;
    li.lParam = (LPARAM)new PROGRAMSLISTSTRUCT(TRUE, item, PROGRAMOPTION_NOAUTOMINIMIZE);
    li.pszText = (LPTSTR)Lang_LoadString(programsListView, IDS_OPTS_PL_NOAUTOMINIMIZE).c_str();
    li.iImage = IMAGE_CHECKBOXUNCHECKED + programOptions.at(PROGRAMOPTION_NOAUTOMINIMIZE);
    ListView_InsertItem(programsListView, &li);
    
    li.iItem = item+4;
    li.iIndent = 1;
    li.lParam = (LPARAM)new PROGRAMSLISTSTRUCT(TRUE, item, PROGRAMOPTION_NOTRAYBUTTON);
    li.pszText = (LPTSTR)Lang_LoadString(programsListView, IDS_OPTS_PL_NOTRAYBUTTON).c_str();
    li.iImage = IMAGE_CHECKBOXUNCHECKED + programOptions.at(PROGRAMOPTION_NOTRAYBUTTON);
    ListView_InsertItem(programsListView, &li);
    
    li.iItem = item+5;
    li.iIndent = 1;
    li.lParam = (LPARAM)new PROGRAMSLISTSTRUCT(TRUE, item, PROGRAMOPTION_NOHOOKMINBUTTON);
    li.pszText = (LPTSTR)Lang_LoadString(programsListView, IDS_OPTS_PL_NOHOOKMINBUTTON).c_str();
    li.iImage = IMAGE_CHECKBOXUNCHECKED + programOptions.at(PROGRAMOPTION_NOHOOKMINBUTTON);
    ListView_InsertItem(programsListView, &li);
    
    ProgramsListView_UpdateSubitems(programsListView, item);
}
void ProgramsListView_RemoveSubitems(HWND programsListView, int item)
{
    for (int item2 = item+5; item2 > item; item2--)
    {
        if (item2 >= ListView_GetItemCount(programsListView))
            continue;
        
        PROGRAMSLISTSTRUCT *pls = (PROGRAMSLISTSTRUCT*)ListView_GetItemParam(programsListView, item2);
        if (!pls->subitem)
            break;
        else
            ListView_DeleteItem(programsListView, item2);
    }
}
void ProgramsListView_Fill(HWND programsListView)
{
    //Remove all the items
    ListView_DeleteAllItems(programsListView);
    
    //Remove all the columns
    int columnsCount = Header_GetItemCount( ListView_GetHeader(programsListView) );
    for (int i = columnsCount-1; i >= 0; i--)
        ListView_DeleteColumn(programsListView, i);
    
    LVCOLUMN columns[] = {
        { LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM, 0, 360, (LPTSTR)_T("Program"), 0,  }
    };
    ListView_InsertColumn(programsListView, 0, &columns[0]);
    
    StringVector programPaths = Options_GetProgramPathsVector(programsListView);
    for (int i = 0; i < programPaths.size(); i++)
    {
        IntVector *programOptions = Options_GetProgramOptionsVector(programsListView, programPaths[i]);
        
        LVITEM li = {0};
        li.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_INDENT|LVIF_PARAM;
        li.iItem = 0;
        li.iIndent = 0;
        li.lParam = (LPARAM)new PROGRAMSLISTSTRUCT(ITEM_COLLAPSED, programPaths[i], FALSE, *programOptions);
        li.pszText = (LPTSTR)programPaths[i].c_str();
        li.iImage = IMAGE_COLLAPSED;
        ListView_InsertItem(programsListView, &li);
    }
}

LRESULT CALLBACK AutoMinTimeEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC originalProc = (WNDPROC)GetProp(hwnd, ORIGINALPROCPROP);
    
    switch(msg)
    {
        case WM_KEYDOWN:
        {
            HWND optionsDlg = GetParent(GetParent(hwnd));
            SetProp(optionsDlg, OPTSMODIFIEDPROP, (HANDLE)TRUE);
        }
        break;
        
        case WM_KILLFOCUS:
        {
            int time = GetDlgItemInt(GetParent(hwnd), GetDlgCtrlID(hwnd), NULL, TRUE);
            if (time == 0)
                SetWindowText(hwnd, _T("1"));
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
LRESULT CALLBACK ProgramsListProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC originalProc = (WNDPROC)GetProp(hwnd, ORIGINALPROCPROP);
    
    switch(msg)
    {
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        {
            POINT cursorPos = {0};
            GetCursorPos(&cursorPos);
            ScreenToClient(hwnd, &cursorPos);
            
            int itemsCount = ListView_GetItemCount(hwnd);
            int columnsCount = Header_GetItemCount( ListView_GetHeader(hwnd) );
            for (int item = 0; item < itemsCount; item++)
            {
                for (int column = 0; column < columnsCount; column++)
                {
                    RECT subItemRect = {0};
                    if (column == 0) {
                        ListView_GetItemRect(hwnd, item, &subItemRect, LVIR_SELECTBOUNDS);
                    }
                    else {
                        ListView_GetSubItemRect(hwnd, item, column, LVIR_LABEL, &subItemRect);
                    }
                    
                    if (PtInRect(&subItemRect, cursorPos))
                    {
                        PROGRAMSLISTSTRUCT *pls = (PROGRAMSLISTSTRUCT*)ListView_GetItemParam(hwnd, item);
                        
                        //Item
                        if (pls->subitem == FALSE)
                        {
                            //Expanded
                            if (pls->state == ITEM_EXPANDED)
                            {
                                pls->state = ITEM_COLLAPSED;
                                
                                //Set the collapsed image
                                ListView_SetItemImage(hwnd, item, column, IMAGE_COLLAPSED);
                                
                                //Remove sub items (options)
                                ProgramsListView_RemoveSubitems(hwnd, item);
                            }
                            //Collapsed
                            else
                            {
                                pls->state = ITEM_EXPANDED;
                                
                                //Set the expanded image
                                ListView_SetItemImage(hwnd, item, column, IMAGE_EXPANDED);
                                
                                //Insert sub items (options)
                                ProgramsListView_InsertSubitems(hwnd, pls->options, item);
                            }
                            
                            continue;
                        }
                        //Subitem
                        else if (!ProgramsListView_IsCheckboxDisabled(hwnd, item))
                        {
                            BOOL checked = ProgramsListView_IsCheckboxChecked(hwnd, item);
                            ListView_SetItemImage(hwnd, item, column, 
                                checked ? IMAGE_CHECKBOXUNCHECKEDHOT : IMAGE_CHECKBOXCHECKEDHOT);
                            checked = !checked;
                            
                            PROGRAMSLISTSTRUCT *plsParent = (PROGRAMSLISTSTRUCT*)
                                ListView_GetItemParam(hwnd, pls->parentItem);
                            plsParent->options[ pls->programOption ] = checked;
                            
                            //Disable/enable other options
                            ProgramsListView_UpdateSubitems(hwnd, pls->parentItem, item);
                            
                            HWND tabDlg = GetParent(hwnd);
                            HWND optionsDlg = GetParent(tabDlg);
                            SetProp(optionsDlg, OPTSMODIFIEDPROP, (HANDLE)TRUE);
                        }
                    }
                }
            }
        }
        break;
        
        case WM_MOUSEMOVE:
        {
            POINT cursorPos = {0};
            GetCursorPos(&cursorPos);
            ScreenToClient(hwnd, &cursorPos);
            
            int itemsCount = ListView_GetItemCount(hwnd);
            int columnsCount = Header_GetItemCount( ListView_GetHeader(hwnd) );
            for (int item = 0; item < itemsCount; item++)
            {
                for (int column = 0; column < columnsCount; column++)
                {
                    RECT subItemRect = {0};
                    if (column == 0)
                        ListView_GetItemRect(hwnd, item, &subItemRect, LVIR_SELECTBOUNDS);
                    else
                        ListView_GetSubItemRect(hwnd, item, column, LVIR_LABEL, &subItemRect);
                    
                    if (PtInRect(&subItemRect, cursorPos))
                    {
                        LVITEM li = {0};
                        li.mask = LVIF_IMAGE|LVIF_INDENT;
                        li.iItem = item;
                        li.iSubItem = column;
                        ListView_GetItem(hwnd, &li);
                        
                        if (li.iIndent == 0)
                            continue;
                        
                        if (li.iImage != IMAGE_CHECKBOXDISABLED)
                        {
                            li.mask = LVIF_IMAGE;
                            if (li.iImage == IMAGE_CHECKBOXCHECKED)
                                li.iImage = IMAGE_CHECKBOXCHECKEDHOT;
                            else if (li.iImage == IMAGE_CHECKBOXUNCHECKED)
                                li.iImage = IMAGE_CHECKBOXUNCHECKEDHOT;
                            ListView_SetItem(hwnd, &li);
                        }
                    }
                    else
                    {
                        LVITEM li = {0};
                        li.mask = LVIF_IMAGE;
                        li.iItem = item;
                        li.iSubItem = column;
                        ListView_GetItem(hwnd, &li);
                        
                        if (li.iImage == IMAGE_CHECKBOXCHECKEDHOT || li.iImage == IMAGE_CHECKBOXUNCHECKEDHOT)
                        {
                            li.iImage = li.iImage == IMAGE_CHECKBOXCHECKEDHOT ? 
                                IMAGE_CHECKBOXCHECKED : IMAGE_CHECKBOXUNCHECKED;
                            ListView_SetItem(hwnd, &li);
                        }
                    }
                }
            }
        }
        break;
        
        case WM_DESTROY:
        {
            ListView_DeleteAllItems(hwnd);
            
            SetWindowLong(hwnd, GWL_WNDPROC, (LONG)originalProc);
            RemoveProp(hwnd, ORIGINALPROCPROP);
        }
        break;
    }
    
    return CallWindowProc(originalProc, hwnd, msg, wParam, lParam);
}

BOOL CALLBACK GeneralOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_LANGUAGECHANGED:
        {
            Lang_DlgItemText(hwnd, IDC_GOD_WINDOWS_RUN, IDS_OPTS_WINDOWSSTART);
            Lang_DlgItemText(hwnd, IDC_GOD_UPD_CHECK, IDS_OPTS_UPD_CHECK);
            Lang_DlgItemText(hwnd, IDC_GOD_UPD_EVERYSTART, IDS_OPTS_UPD_CHECK_EVERYSTART);
            Lang_DlgItemText(hwnd, IDC_GOD_UPD_EVERYDAY, IDS_OPTS_UPD_CHECK_EVERYDAY);
            Lang_DlgItemText(hwnd, IDC_GOD_UPD_EVERYWEEK, IDS_OPTS_UPD_CHECK_WEEKLY);
            Lang_DlgItemText(hwnd, IDH_GOD_LANGUAGE, IDS_OPTS_LANGUAGE);
        }
        break;
        
        case WM_COMMAND:
        {
            GeneralOptionsDlg_Update(hwnd);
            
            switch(LOWORD(wParam))
            {
                case IDC_GOD_WINDOWS_RUN:
                case IDC_GOD_UPD_CHECK:
                case IDC_GOD_UPD_EVERYSTART:
                case IDC_GOD_UPD_EVERYDAY:
                case IDC_GOD_UPD_EVERYWEEK:
                case IDC_GOD_LANGUAGE:
                {
                    HWND optionsDlg = GetParent(hwnd);
                    SetProp(optionsDlg, OPTSMODIFIEDPROP, (HANDLE)TRUE);
                }
                break;
            }
        }
        break;
        
        case WM_INITDIALOG:
        {
            HWND languageCB = GetDlgItem(hwnd, IDC_GOD_LANGUAGE);
            LanguageCombobox_Refresh(languageCB);
        }
        break;
        
        case WM_DESTROY:
        {
            HWND languageCB = GetDlgItem(hwnd, IDC_GOD_LANGUAGE);
            LanguageCombobox_Free(languageCB);
        }
        break;
    }
    
    return FALSE;
}
BOOL CALLBACK TrayOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_LANGUAGECHANGED:
        {
            Lang_DlgItemText(hwnd, IDC_TOD_TE_MINONCLOSE, IDS_OPTS_MINONCLOSE);
            Lang_DlgItemText(hwnd, IDC_TOD_TE_HIDEONMIN, IDS_OPTS_HIDEONMIN);
            Lang_DlgItemText(hwnd, IDH_TOD_TE_HIDEONMIN, IDS_OPTS_HIDEONMIN_HELP);
            Lang_DlgItemText(hwnd, IDH_TOD_OTHERPROGS, IDS_OPTS_OTHERPROGS);
            Lang_DlgItemText(hwnd, IDC_TOD_OP_NOICON, IDS_OPTS_TRAY_NOICON);
            Lang_DlgItemText(hwnd, IDC_TOD_OP_PWDPROTECT, IDS_OPTS_TRAY_PWDPROTECT);
            Lang_DlgItemText(hwnd, IDC_TOD_OP_GROUPICON, IDS_OPTS_TRAY_GROUPICON);
        }
        break;
        
        case WM_COMMAND:
        {
            TrayOptionsDlg_Update(hwnd);
            
            switch(LOWORD(wParam))
            {
                case IDC_TOD_TE_MINONCLOSE:
                case IDC_TOD_TE_HIDEONMIN:
                case IDC_TOD_OP_NOICON:
                case IDC_TOD_OP_PWDPROTECT:
                case IDC_TOD_OP_GROUPICON:
                {
                    HWND optionsDlg = GetParent(hwnd);
                    SetProp(optionsDlg, OPTSMODIFIEDPROP, (HANDLE)TRUE);
                }
                break;
            }
        }
        break;
        
        case WM_INITDIALOG:
        {
            // Check if a window (it need only one window) has currently a password
            // This is to prevent some1 from avoiding the password dialog.
            WindowsVector *windowsVec = Windows_Get(hwnd);
            BOOL enablePasswordOption = TRUE;
            for (int i = 0; i<windowsVec->size(); i++)
            {
                if (windowsVec->at(i).hwnd == NULL)
                   break;
                
                if (windowsVec->at(i).password == _T(""))
                {
                    enablePasswordOption = FALSE;
                    break;
                }
            }
            
            // If so, disable the password option
            EnableDlgItem(hwnd, IDC_TOD_OP_PWDPROTECT, enablePasswordOption);
            
            HFONT boldFont = BoldFont_Get(GetParent(hwnd));
            SetDlgItemFont(hwnd, IDH_TOD_MAIN, boldFont);
            SetDlgItemFont(hwnd, IDH_TOD_OTHERPROGS, boldFont);
        }
        break;
    }
    
    return FALSE;
}
BOOL CALLBACK Tray2OptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_LANGUAGECHANGED:
        {
            Lang_DlgItemText(hwnd, IDH_T2OD_AUTOMINIMIZE, IDS_OPTS_AUTOMINIMIZE);
            Lang_DlgItemText(hwnd, IDC_T2OD_AUTOMINIMIZE, IDS_OPTS_AUTOMIN);
            Lang_DlgItemText(hwnd, IDH_T2OD_MINFROMPROGR, IDS_OPTS_MINFROMPROGRS);
            Lang_DlgItemText(hwnd, IDC_T2OD_MFP_NONE, IDS_OPTS_MFP_NONE);
            Lang_DlgItemText(hwnd, IDC_T2OD_MFP_TRAYBUTTON, IDS_OPTS_MFP_TRAYBUTTON);
            Lang_DlgItemText(hwnd, IDC_T2OD_MFP_MINBUTTON, IDS_OPTS_MFP_MINBUTTON);
            Lang_DlgItemText(hwnd, IDC_T2OD_MFP_MIDDLECLICK, IDS_OPTS_MFP_MIDDLECLICK);
            
            HWND timeType = GetDlgItem(hwnd, IDC_T2OD_AM_TIMETYPE);
            int curSel = ComboBox_GetCurSel(timeType);
            ComboBox_ResetContent(timeType);
            ComboBox_InsertString(timeType, 0, Lang_LoadString(hwnd, IDS_OPTS_AUTOMIN_TYPE_SECS).c_str());
            ComboBox_InsertString(timeType, 1, Lang_LoadString(hwnd, IDS_OPTS_AUTOMIN_TYPE_MINS).c_str());
            ComboBox_SetCurSel(timeType, curSel);
            
            HWND optionsDlg = GetParent(hwnd);
            HWND tooltip = (HWND)GetProp(optionsDlg, TOOLTIPPROP);
            
            //Remove tooltips
            {
                TOOLINFO ti = {0};
                ti.cbSize = sizeof(TOOLINFO);
                ti.hwnd = hwnd;
                
                ti.uId = (UINT_PTR)GetDlgItem(hwnd, IDC_T2OD_MFP_MINBUTTON);
                SendMessage(tooltip, TTM_DELTOOL, 0, (LPARAM)&ti);
                
                ti.uId = (UINT_PTR)GetDlgItem(hwnd, IDC_T2OD_MFP_TRAYBUTTON);
                SendMessage(tooltip, TTM_DELTOOL, 0, (LPARAM)&ti);
            }
            
            //Add tooltips
            {
                TOOLINFO ti = {0};
                ti.cbSize = sizeof(TOOLINFO);
                ti.hwnd = hwnd;
                ti.uFlags = TTF_IDISHWND|TTF_SUBCLASS;
                
                ti.uId = (UINT_PTR)GetDlgItem(hwnd, IDC_T2OD_MFP_MINBUTTON);
                ti.lpszText = (LPTSTR)Lang_LoadString(hwnd, IDS_OPTS_MFP_MINBUTTON_TT).c_str();
                SendMessage(tooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
                
                ti.uId = (UINT_PTR)GetDlgItem(hwnd, IDC_T2OD_MFP_TRAYBUTTON);
                ti.lpszText = (LPTSTR)Lang_LoadString(hwnd, IDS_OPTS_MFP_TRAYBUTTON_TT).c_str();
                SendMessage(tooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
            }
        }
        break;
        
        case WM_INITDIALOG:
        {
            SendMessage(hwnd, WM_LANGUAGECHANGED, 0,0);
            
            HFONT boldFont = BoldFont_Get(GetParent(hwnd));
            SetDlgItemFont(hwnd, IDH_T2OD_AUTOMINIMIZE, boldFont);
            SetDlgItemFont(hwnd, IDH_T2OD_MINFROMPROGR, boldFont);
            
            HWND timeEdit = GetDlgItem(hwnd, IDC_T2OD_AM_TIME);
            WNDPROC originalProc = (WNDPROC)SetWindowLong(timeEdit, GWL_WNDPROC, (LONG)AutoMinTimeEditProc);
            SetProp(timeEdit, ORIGINALPROCPROP, (HANDLE)originalProc);
        }
        break;
        
        case WM_COMMAND:
        {
            Tray2OptionsDlg_Update(hwnd);
            
            switch(LOWORD(wParam))
            {
                case IDC_T2OD_AUTOMINIMIZE:
                case IDC_T2OD_AM_TIMETYPE:
                case IDC_T2OD_MFP_NONE:
                case IDC_T2OD_MFP_TRAYBUTTON:
                case IDC_T2OD_MFP_MINBUTTON:
                case IDC_T2OD_MFP_MIDDLECLICK:
                {
                    HWND optionsDlg = GetParent(hwnd);
                    SetProp(optionsDlg, OPTSMODIFIEDPROP, (HANDLE)TRUE);
                }
                break;
            }
        }
        break;
    }
    
    return FALSE;
}
BOOL CALLBACK ProgramsOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_SHOWWINDOW:
        {
            //Refresh the listview each time the dialog is shown
            HWND programsListView = GetDlgItem(hwnd, IDC_POD_PROGRAMSLIST);
            ProgramsListView_Fill(programsListView);
        }
        break;
        
        case WM_LANGUAGECHANGED:
            //Lang_DlgItemText(hwnd, IDC_HOD_KBH_ENABLE, IDS_OPTS_KBHOTKEY);
            break;
        
        case WM_NOTIFY:
        {
            NMHDR *nmh = (NMHDR*)lParam;
            switch(nmh->code)
            {
                case LVN_DELETEITEM:
                {
                    HWND programsListView = nmh->hwndFrom;
                    NMLISTVIEW *nmlv = (NMLISTVIEW*)lParam;
                    
                    LVITEM li = {0};
                    li.mask = LVIF_PARAM;
                    li.iItem = nmlv->iItem;
                    ListView_GetItem(programsListView, &li);
                    
                    PROGRAMSLISTSTRUCT *pls = (PROGRAMSLISTSTRUCT*)li.lParam;
                    if (pls)
                        delete pls;
                }
                break;
            }
        }
        break;
        
        case WM_INITDIALOG:
        {
            HWND programsListView = GetDlgItem(hwnd, IDC_POD_PROGRAMSLIST);
            
            WNDPROC originalProc = (WNDPROC)SetWindowLong(programsListView, GWL_WNDPROC, (LONG)ProgramsListProc);
            SetProp(programsListView, ORIGINALPROCPROP, (HANDLE)originalProc);
            
            ListView_SetExtendedListViewStyle(programsListView, LVS_EX_SUBITEMIMAGES);
            ProgramsListView_FillImageList(programsListView);
            ProgramsListView_Fill(programsListView);
        }
        break;
    }
    
    return FALSE;
}
BOOL CALLBACK HotkeyOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_LANGUAGECHANGED:
            Lang_DlgItemText(hwnd, IDC_HOD_KBH_ENABLE, IDS_OPTS_KBHOTKEY);
            break;
        
        case WM_COMMAND:
        {
            HotkeysOptionsDlg_Update(hwnd);
            
            switch(LOWORD(wParam))
            {
                case IDC_HOD_KBH_F7:
                case IDC_HOD_KBH_WINQ:
                case IDC_HOD_KBH_WINS:
                case IDC_HOD_KBH_CTRLQ:
                {
                    HWND optionsDlg = GetParent(hwnd);
                    SetProp(optionsDlg, OPTSMODIFIEDPROP, (HANDLE)TRUE);
                }
                break;
            }
        }
        break;
    }
    
    return FALSE;
}

BOOL CALLBACK OptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int oldTabSel = 0;
    
    switch (msg)
    {
        case WM_LBUTTONDOWN:
            MainDlg_MoveOnClick(hwnd);
            break;
        
        case WM_LANGUAGECHANGED:
        {
            Lang_DlgItemText(hwnd, IDC_OPTS_DEFAULT, IDS_DLG_DEFAULT);
            Lang_DlgItemText(hwnd, IDC_OPTS_APPLY, IDS_DLG_APPLY);
            
            HWND tab = GetDlgItem(hwnd, IDC_OPTS_TAB);
            TCITEM ti = {0};
            ti.mask = TCIF_TEXT;
            {
                ti.pszText = (LPTSTR)Lang_LoadString(tab, IDS_OPT_TAB_GEN).c_str();
                TabCtrl_SetItem(tab, 0, &ti);
                
                ti.pszText = (LPTSTR)Lang_LoadString(tab, IDS_OPT_TAB_TRAY).c_str();
                TabCtrl_SetItem(tab, 1, &ti);
                
                ti.pszText = (LPTSTR)Lang_LoadString(tab, IDS_OPT_TAB_TRAY2).c_str();
                TabCtrl_SetItem(tab, 2, &ti);
                
                ti.pszText = (LPTSTR)Lang_LoadString(tab, IDS_OPT_TAB_PROGRAMS).c_str();
                TabCtrl_SetItem(tab, 3, &ti);
                
                ti.pszText = (LPTSTR)Lang_LoadString(tab, IDS_OPT_TAB_HK).c_str();
                TabCtrl_SetItem(tab, 4, &ti);
            }
            
            int newTabSel = TabCtrl_GetCurSel(tab);
            HWND newDlg = (HWND)TabCtrl_GetItemParam(tab, newTabSel);
            ShowWindow(newDlg, SW_HIDE);
            ShowWindow(newDlg, SW_SHOW);
            
            OptionsDialogs *dlgs = (OptionsDialogs*)GetProp(hwnd, OPTSDIALOGSPROP);
            SendMessage((*dlgs)[IDD_OPTS_GENDLG], WM_LANGUAGECHANGED, 0,0);
            SendMessage((*dlgs)[IDD_OPTS_TRAYDLG], WM_LANGUAGECHANGED, 0,0);
            SendMessage((*dlgs)[IDD_OPTS_TRAY2DLG], WM_LANGUAGECHANGED, 0,0);
            SendMessage((*dlgs)[IDD_OPTS_PROGRAMSDLG], WM_LANGUAGECHANGED, 0,0);
            SendMessage((*dlgs)[IDD_OPTS_HOTKEYSDLG], WM_LANGUAGECHANGED, 0,0);
        }
        break;
        
        case WM_DESTROY:
        {
            RemoveProp(hwnd, OPTSMODIFIEDPROP);
            
            OptionsDialogs *dlgs = (OptionsDialogs*)RemoveProp(hwnd, OPTSDIALOGSPROP);
            delete dlgs;
            
            //HWND tab = GetDlgItem(hwnd, IDC_OPTS_TAB);
            BoldFont_Delete(hwnd);
        }
        break;
        
        case WM_SHOWWINDOW:
        {
            //Refresh the programs listview each time the options dialog is shown
            OptionsDialogs *dlgs = (OptionsDialogs*)GetProp(hwnd, OPTSDIALOGSPROP);
            HWND programsDlg = (*dlgs)[IDD_OPTS_PROGRAMSDLG];
            ProgramsListView_Fill(GetDlgItem(programsDlg, IDC_POD_PROGRAMSLIST));
            
            if (wParam == FALSE)
            {
                if (GetProp(hwnd, OPTSMODIFIEDPROP))
                {
                    if(IDYES == YesNoBox(hwnd, IDS_OPTS_CHANGED))
                        SendDlgItemMessage(hwnd, IDC_OPTS_APPLY, BM_CLICK, 0, 0);
                }
                
                SetProp(hwnd, OPTSMODIFIEDPROP, (HANDLE)FALSE);
            }
        }
        break;
        
        case WM_INITDIALOG:
        {
            HWND tooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, 
                WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP, CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT, 
                hwnd, 0, GetModuleHandle(NULL), 0);
            SendMessage(tooltip, TTM_SETMAXTIPWIDTH, 1, MAX_PATH);
            SetProp(hwnd, TOOLTIPPROP, tooltip);
            
            HWND tab = GetDlgItem(hwnd, IDC_OPTS_TAB);
            BoldFont_Create(hwnd);
            
            HINSTANCE inst = GetModuleHandle(NULL);
            HWND genOptsDlg = CreateDialog(inst, MAKEINTRESOURCE(IDD_OPTS_GENDLG), hwnd, GeneralOptionsDlgProc);
            HWND trayOptsDlg = CreateDialog(inst, MAKEINTRESOURCE(IDD_OPTS_TRAYDLG), hwnd, TrayOptionsDlgProc);
            HWND tray2OptsDlg = CreateDialog(inst, MAKEINTRESOURCE(IDD_OPTS_TRAY2DLG), hwnd, Tray2OptionsDlgProc);
            HWND programsOptsDlg = CreateDialog(inst, MAKEINTRESOURCE(IDD_OPTS_PROGRAMSDLG), hwnd, ProgramsOptionsDlgProc);
            HWND hkOptsDlg = CreateDialog(inst, MAKEINTRESOURCE(IDD_OPTS_HOTKEYSDLG), hwnd, HotkeyOptionsDlgProc);
            TabDlg_Init(tab, genOptsDlg);
            TabDlg_Init(tab, trayOptsDlg);
            TabDlg_Init(tab, tray2OptsDlg);
            TabDlg_Init(tab, programsOptsDlg);
            TabDlg_Init(tab, hkOptsDlg);
            TabDlg_SetThemeBkgnd(genOptsDlg);
            TabDlg_SetThemeBkgnd(trayOptsDlg);
            TabDlg_SetThemeBkgnd(tray2OptsDlg);
            TabDlg_SetThemeBkgnd(programsOptsDlg);
            TabDlg_SetThemeBkgnd(hkOptsDlg);
            
            OptionsDialogs *dlgs = new OptionsDialogs;
            (*dlgs)[IDD_OPTS_GENDLG] = genOptsDlg;
            (*dlgs)[IDD_OPTS_TRAYDLG] = trayOptsDlg;
            (*dlgs)[IDD_OPTS_TRAY2DLG] = tray2OptsDlg;
            (*dlgs)[IDD_OPTS_PROGRAMSDLG] = programsOptsDlg;
            (*dlgs)[IDD_OPTS_HOTKEYSDLG] = hkOptsDlg;
            SetProp(hwnd, OPTSDIALOGSPROP, (HANDLE)dlgs);
            
            TabCtrl_Init(tab);
            SendMessage(hwnd, WM_LANGUAGECHANGED, 0,0);
            
            
            GeneralOptionsDlg_Restore(genOptsDlg);
            TrayOptionsDlg_Restore(trayOptsDlg);
            Tray2OptionsDlg_Restore(tray2OptsDlg);
            ProgramsOptionsDlg_Restore(programsOptsDlg);
            HotkeysOptionsDlg_Restore(hkOptsDlg);
            
            GeneralOptionsDlg_Update(genOptsDlg);
            TrayOptionsDlg_Update(trayOptsDlg);
            Tray2OptionsDlg_Update(tray2OptsDlg);
            ProgramsOptionsDlg_Update(programsOptsDlg);
            HotkeysOptionsDlg_Update(hkOptsDlg);
        }
        break;
        
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_OPTS_DEFAULT:
                {
                    OptionsDialogs *dlgs = (OptionsDialogs*)GetProp(hwnd, OPTSDIALOGSPROP);
                    
                    HWND getOptsDlg = (*dlgs)[IDD_OPTS_GENDLG];
                    {
                        CheckDlgButton(getOptsDlg, IDC_GOD_WINDOWS_RUN, BST_UNCHECKED);
                        CheckDlgButton(getOptsDlg, IDC_GOD_UPD_CHECK, BST_UNCHECKED);
                        ComboBox_SetCurSel(GetDlgItem(getOptsDlg, IDC_GOD_LANGUAGE), 3);
                    }
                    
                    HWND trayOptsDlg = (*dlgs)[IDD_OPTS_TRAYDLG];
                    {
                        CheckDlgButton(trayOptsDlg, IDC_TOD_TE_MINONCLOSE, BST_UNCHECKED);
                        CheckDlgButton(trayOptsDlg, IDC_TOD_TE_HIDEONMIN, BST_UNCHECKED);
                        CheckDlgButton(trayOptsDlg, IDC_TOD_OP_NOICON, BST_UNCHECKED);
                        CheckDlgButton(trayOptsDlg, IDC_TOD_OP_PWDPROTECT, BST_UNCHECKED);
                    }
                    
                    HWND tray2OptsDlg = (*dlgs)[IDD_OPTS_TRAY2DLG];
                    {
                        CheckDlgButton(tray2OptsDlg, IDC_T2OD_AUTOMINIMIZE, BST_UNCHECKED);
                        CheckRadioButton(tray2OptsDlg, IDC_T2OD_MFP_NONE, IDC_T2OD_MFP_MINBUTTON, IDC_T2OD_MFP_NONE);
                    }
                    
                    HWND programsOptsDlg = (*dlgs)[IDD_OPTS_PROGRAMSDLG];
                    
                    HWND hkOptsDlg = (*dlgs)[IDD_OPTS_HOTKEYSDLG];
                    {
                        CheckDlgButton(hkOptsDlg, IDC_HOD_KBH_ENABLE, BST_UNCHECKED);
                        CheckRadioButton(hkOptsDlg, IDC_HOD_KBH_F7, IDC_HOD_KBH_CTRLQ, IDC_HOD_KBH_F7);
                    }
                    
                    GeneralOptionsDlg_Update(getOptsDlg);
                    TrayOptionsDlg_Update(trayOptsDlg);
                    Tray2OptionsDlg_Update(tray2OptsDlg);
                    ProgramsOptionsDlg_Update(programsOptsDlg);
                    HotkeysOptionsDlg_Update(hkOptsDlg);
                    
                    SetProp(hwnd, OPTSMODIFIEDPROP, (HANDLE)TRUE);
                }
                break;
                
                case IDC_OPTS_APPLY:
                {
                    SetProp(hwnd, OPTSMODIFIEDPROP, (HANDLE)FALSE);
                    
                    OptionsDialogs *dlgs = (OptionsDialogs*)GetProp(hwnd, OPTSDIALOGSPROP);
                    GeneralOptionsDlg_Save( (*dlgs)[IDD_OPTS_GENDLG] );
                    TrayOptionsDlg_Save( (*dlgs)[IDD_OPTS_TRAYDLG] );
                    Tray2OptionsDlg_Save( (*dlgs)[IDD_OPTS_TRAY2DLG] );
                    ProgramsOptionsDlg_Save( (*dlgs)[IDD_OPTS_PROGRAMSDLG] );
                    HotkeysOptionsDlg_Save( (*dlgs)[IDD_OPTS_HOTKEYSDLG] );
                }
                break;
            }
        }
        break;
        
        case WM_NOTIFY:
        {
            NMHDR *nmh = (NMHDR*)lParam;
            switch(nmh->code)
            {
                case TCN_SELCHANGE:
                {
                    HWND tab = GetDlgItem(hwnd, IDC_OPTS_TAB);
                    int newTabSel = TabCtrl_GetCurSel(tab);
                    
                    HWND oldDlg = (HWND)TabCtrl_GetItemParam(tab, oldTabSel);
                    HWND newDlg = (HWND)TabCtrl_GetItemParam(tab, newTabSel);
                    
                    ShowWindow(oldDlg, SW_HIDE);
                    ShowWindow(newDlg, SW_SHOW);
                    
                    oldTabSel = newTabSel;
                }
                break;
            }
        }
        break;
    }
    
    return FALSE;
}
