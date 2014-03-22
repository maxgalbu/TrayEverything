#include "resource.h"
#include "LangFuncs.h"
#include <shlwapi.h>
#include <uxtheme.h>
#include <wininet.h>
#include <olectl.h>
#include <ole2.h>

unsigned long currentId = 0;
unsigned long GetUniqueId()
{
    return currentId++;
}

tstring lstrprintf(HWND hwnd, int formatId, ...)
{
    va_list argList;
    va_start(argList, formatId);
    
    TCHAR text[1024] = _T("");
    _vsntprintf(text, 1024, Lang_LoadString(hwnd, formatId).c_str(), argList);
    
    va_end(argList);
    return text;
}
tstring strprintf(LPCTSTR format, ...)
{
    va_list argList;
    va_start(argList, format);
    
    TCHAR text[1024] = _T("");
    _vsntprintf(text, 1024, format, argList);
    
    va_end(argList);
    return text;
}
void trim(tstring &str)
{
    TCHAR space = _T(' ');
    int pos1 = str.find_first_not_of(space);
    int pos2 = str.find_last_not_of(space);
    
    if (pos1 == tstring::npos)
        pos1 = 0;
    
    if (pos2 == tstring::npos)
        pos2 = str.length() - 1;
    else
        pos2 = pos2 - pos1 + 1;
    
    str = str.substr(pos1, pos2);
}
StringVector explode(tstring stringToExplode, const tstring& separator) 
{
    trim(stringToExplode);
    std::vector<tstring> explodedStrings;
    
    int sepPos = 0;
    while( (sepPos = stringToExplode.find(separator, 0)) != tstring::npos )
    {
        explodedStrings.push_back( stringToExplode.substr(0, sepPos) );
        stringToExplode.erase(0, sepPos + separator.size());
    }
    
    if(stringToExplode != _T(""))
        explodedStrings.push_back(stringToExplode);
    
    return explodedStrings;
}

HBITMAP LoadImageFromResource(UINT resId, LPCTSTR resType)
{
    HBITMAP bitmap = NULL;
    
    HRSRC resHandle = FindResource(NULL, MAKEINTRESOURCE(resId), resType);
    if (resHandle)
    {
        HGLOBAL memData = LoadResource(NULL, resHandle);
        if (memData)
        {
            DWORD resSize = SizeofResource(NULL, resHandle);
            
            HGLOBAL globalData = GlobalAlloc(0, resSize);
            LPVOID lockedData = GlobalLock(globalData);
            memcpy(lockedData, memData, resSize);
            GlobalUnlock(globalData);
            
            LPSTREAM pStream = NULL;
            CreateStreamOnHGlobal(globalData, FALSE, &pStream);
            
            LPPICTURE pPicture = NULL;
            OleLoadPicture(pStream, resSize, TRUE, IID_IPicture, (LPVOID*)&pPicture);
            
            if (pPicture)
            {
                HBITMAP handle = NULL;
                pPicture->get_Handle((unsigned int*)&handle);
                bitmap = (HBITMAP)CopyImage(handle, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG);
                
                pPicture->Release();
            }
            
            pStream->Release(); 
            GlobalFree(globalData);
        }
    }
    
    return bitmap;
}

int MsgBox(HWND hwnd, UINT boxFlags, LPCTSTR title, LPCTSTR fmtString, va_list argList)
{
    TCHAR errorText[1024];
    _vsntprintf(errorText, 1024, fmtString, argList);
    
    return MessageBox(hwnd, errorText, title, boxFlags|MB_TOPMOST);
}
int MsgBox(HWND hwnd, UINT boxFlags, LPCTSTR title, LPCTSTR fmtString, ...)
{
    va_list argList;
    
    va_start(argList, fmtString);
    int choice = MsgBox(hwnd, boxFlags, title, fmtString, argList);
    va_end(argList);
    
    return choice;
}
void ErrorBox(HWND hwnd, BOOL isCritical, LPCTSTR tfmtString, ...)
{
    va_list argList;
    
    tstring fmtString = _T("");
    if (isCritical) fmtString += _T("CRITICAL ERROR: ");
    fmtString += tfmtString;
    
    va_start(argList, tfmtString);
    MsgBox(hwnd, MB_OK|MB_ICONERROR, Lang_LoadString(hwnd, IDS_ERROR).c_str(), 
        fmtString.c_str(), argList);
    va_end(argList);
}
void ErrorBox(HWND hwnd, BOOL isCritical, UINT fmtStringId, ...)
{
    va_list argList;
    
    tstring fmtString = _T("");
    if (isCritical) fmtString += _T("CRITICAL ERROR: ");
    fmtString += Lang_LoadString(hwnd, fmtStringId);
    
    va_start(argList, fmtStringId);
    MsgBox(hwnd, MB_OK|MB_ICONERROR, Lang_LoadString(hwnd, IDS_ERROR).c_str(), 
        fmtString.c_str(), argList);
    va_end(argList);
}
int YesNoBox(HWND hwnd, LPCTSTR fmtString, ...)
{
    va_list argList;
    
    va_start(argList, fmtString);
    int choice = MsgBox(hwnd, MB_YESNO|MB_ICONQUESTION, 
        Lang_LoadString(hwnd, IDS_YESNO).c_str(), fmtString, argList);
    va_end(argList);
    
    return choice;
}
int YesNoBox(HWND hwnd, UINT fmtStringId, ...)
{
    va_list argList;
    
    va_start(argList, fmtStringId);
    int choice = MsgBox(hwnd, MB_YESNO|MB_ICONQUESTION, Lang_LoadString(hwnd, IDS_YESNO).c_str(), 
        Lang_LoadString(hwnd, fmtStringId).c_str(), argList);
    va_end(argList);
    
    return choice;
}
int YesNoCancelBox(HWND hwnd, LPCTSTR fmtString, ...)
{
    va_list argList;
    
    va_start(argList, fmtString);
    int choice = MsgBox(hwnd, MB_YESNOCANCEL|MB_ICONQUESTION, 
        Lang_LoadString(hwnd, IDS_YESNO).c_str(), fmtString, argList);
    va_end(argList);
    
    return choice;
}
int YesNoCancelBox(HWND hwnd, UINT fmtStringId, ...)
{
    va_list argList;
    
    va_start(argList, fmtStringId);
    int choice = MsgBox(hwnd, MB_YESNOCANCEL|MB_ICONQUESTION, Lang_LoadString(hwnd, IDS_YESNO).c_str(), 
        Lang_LoadString(hwnd, fmtStringId).c_str(), argList);
    va_end(argList);
    
    return choice;
}
void InfoBox(HWND hwnd, LPCTSTR fmtString, ...)
{
    va_list argList;
    
    va_start(argList, fmtString);
    MsgBox(hwnd, MB_OK|MB_ICONINFORMATION, Lang_LoadString(hwnd, IDS_INFO).c_str(), fmtString, argList);
    va_end(argList);
}
void InfoBox(HWND hwnd, UINT fmtStringId, ...)
{
    va_list argList;
    
    va_start(argList, fmtStringId);
    MsgBox(hwnd, MB_OK|MB_ICONINFORMATION, Lang_LoadString(hwnd, IDS_INFO).c_str(), 
        Lang_LoadString(hwnd, fmtStringId).c_str(), argList);
    va_end(argList);
}

tstring FileName(const tstring& filePath)
{
    return filePath.substr( filePath.find_last_of( _T('\\'), filePath.size() ) +1 );
}
tstring FileExt(const tstring& filePath)
{
    int dotPos = filePath.find_last_of( _T('.') );
    if (dotPos == tstring::npos)
        return _T("");
    return filePath.substr( dotPos+1 );
}
tstring DirName(const tstring& filePath)
{
    return filePath.substr( 0, filePath.find_last_of( _T('\\'), filePath.size() ) );
}
BOOL IsWindowsXp()
{
    static BOOL isWindowsXp = -1;
    if (isWindowsXp != -1)
        return isWindowsXp;
    
    //Default value
    isWindowsXp = FALSE;
    
    OSVERSIONINFO ovi = {0};
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&ovi);
    
    if (ovi.dwMajorVersion > 5 || (ovi.dwMajorVersion == 5 && ovi.dwMinorVersion >= 1))
        isWindowsXp = TRUE;
    
    return isWindowsXp;
}
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
BOOL FileExists(const tstring& fileName)
{
    return (GetFileAttributes(fileName.c_str()) != INVALID_FILE_ATTRIBUTES);
}
tstring FormatLastError()
{
    DWORD lastError = GetLastError();
    TCHAR errorText[256] = _T("");
    
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        errorText, 256, NULL);
    
    return errorText;
}
tstring FormatBytes(int bytes)
{
    if (bytes < 1024)
        return strprintf(_T("%d bytes"), bytes);
    else
    {
        if (bytes < 1024*1024)
            return strprintf(_T("%.2f KB"), (double)bytes/1024);
        else
        {
            if (bytes < 1024*1024*1024)
                return strprintf(_T("%.2f MB"), (double)bytes/1024/1024);
        }
    }
    
    //Unreacheable
    return _T("");
}
tstring FormatLastInternetError()
{
    TCHAR errorText[1024] = _T("");
    DWORD errorTextSize = sizeof(errorText);
    DWORD errorCode = 0;
    InternetGetLastResponseInfo(&errorCode, errorText, &errorTextSize);
    
    return errorText;
}
void RegisterDialogClass(LPCTSTR newClassName)
{
	WNDCLASSEX wc;
    wc.cbSize = sizeof(wc);
    
	//Get the class structure for the system dialog class
	GetClassInfoEx(0, _T("#32770"), &wc);
    
	//Make sure our new class does not conflict
	wc.style &= ~CS_GLOBALCLASS;
    
	//Register an identical dialog class, but with a new name!
	wc.lpszClassName = newClassName;
    
	RegisterClassEx(&wc);
}

void ListView_SetItemImage(HWND listView, int item, int subitem, int image)
{
    LVITEM li = {0};
    li.mask = LVIF_IMAGE;
    li.iItem = item;
    li.iSubItem = subitem;
    li.iImage = image;
    ListView_SetItem(listView, &li);
}
int ListView_GetItemImage(HWND listView, int item, int subitem)
{
    LVITEM li = {0};
    li.mask = LVIF_IMAGE;
    li.iItem = item;
    li.iSubItem = subitem;
    ListView_GetItem(listView, &li);
    
    return li.iImage;
}
LPARAM ListView_GetItemParam(HWND listView, int index)
{
    LVITEM li = {0};
    li.mask = LVIF_PARAM;
    li.iItem = index;
    ListView_GetItem(listView, &li);
    
    return li.lParam;
}
void ListView_SetHeaderSortImage(HWND listView, int columnIndex, BOOL isAscending)
{
    HWND header = ListView_GetHeader(listView);
    BOOL isCommonControlVersion6 = IsCommCtrlVersion6();
    int columnCount = Header_GetItemCount(header);
    
    for (int i = 0; i<columnCount; i++)
    {
        HDITEM hi = {0};
        hi.mask = HDI_FORMAT | (isCommonControlVersion6 ? 0 : HDI_BITMAP);
        Header_GetItem(header, i, &hi);
        
        //Set sort image to this column
        if (i == columnIndex)
        {
            if (isCommonControlVersion6)
            {
                hi.fmt &= ~(HDF_SORTDOWN|HDF_SORTUP);
                hi.fmt |= isAscending ? HDF_SORTUP : HDF_SORTDOWN;
            }
            else
            {
                UINT bitmapID = isAscending ? IDB_ARROWUP : IDB_ARROWDOWN;
                
                //If there's a bitmap, let's delete it.
                if (hi.hbm)
                    DeleteObject(hi.hbm);
                
                hi.fmt |= HDF_BITMAP|HDF_BITMAP_ON_RIGHT;
                hi.hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(bitmapID), IMAGE_BITMAP, 0,0, LR_LOADMAP3DCOLORS);
            }
        }
        //Remove sort image (if exists)
        //from other columns.
        else
        {
            if (isCommonControlVersion6)
                hi.fmt &= ~(HDF_SORTDOWN|HDF_SORTUP);
            else
            {
                //If there's a bitmap, let's delete it.
                if (hi.hbm)
                    DeleteObject(hi.hbm);
                
                hi.mask &= ~HDI_BITMAP;
                hi.fmt &= ~(HDF_BITMAP|HDF_BITMAP_ON_RIGHT);
            }
        }
        
        Header_SetItem(header, i, &hi);
    }
}
void TabDlg_SetThemeBkgnd(HWND tabDlg)
{
    if (IsCommCtrlVersion6())
    {
        HMODULE themeDll = LoadLibrary( _T("uxtheme.dll") );
        if (themeDll)
        {
            typedef HRESULT (WINAPI *PNFENABLETHEMEDIALOGTEXTURE)(HWND, DWORD);
            PNFENABLETHEMEDIALOGTEXTURE pEnableThemeDialogTexture =
                (PNFENABLETHEMEDIALOGTEXTURE)GetProcAddress(themeDll, "EnableThemeDialogTexture" );
            
            if (pEnableThemeDialogTexture)
                pEnableThemeDialogTexture(tabDlg, ETDT_ENABLETAB);
            
            FreeLibrary(themeDll);
        }
    }
}
LPARAM TabCtrl_GetItemParam(HWND hwndTab, int itemIndex)
{
    TCITEM ti = {0};
    ti.mask = TCIF_PARAM;
    TabCtrl_GetItem(hwndTab, itemIndex, &ti);
    
    return ti.lParam;
}

tstring GetProgramFullPath()
{
    static TCHAR currentPath[1024] = _T("");
    if (!_tcscmp(currentPath, _T("")))
        GetModuleFileName(NULL, currentPath, 1024);
    
    return currentPath;
}
tstring GetProgramDir()
{
    tstring filePath = GetProgramFullPath();
    return filePath.substr(0, 1 + filePath.find_last_of('\\', filePath.size()));
}
HWND GetMainWindow(HWND hwnd)
{
    return GetAncestor(hwnd, GA_ROOTOWNER);
}
HWND GetWindowsList(HWND hwnd)
{
    return GetDlgItem(GetMainWindow(hwnd), IDC_WINDOWSLIST);
}
HWND GetShellWindow()
{
    return FindWindow(_T("Shell_TrayWnd"), NULL);
}
HWND GetTrayWindow()
{
    HWND shell = GetShellWindow();
    HWND notify = FindWindowEx(shell, NULL, _T("TrayNotifyWnd"), NULL);
    HWND sysPager = FindWindowEx(notify, NULL, _T("SysPager"), NULL);
    HWND tray = FindWindowEx(sysPager, NULL, _T("ToolbarWindow32"), NULL);
    
    return tray;
}

//Font functions
void BoldFont_Create(HWND hwnd)
{
    HFONT defaultFont = GetStockFont(DEFAULT_GUI_FONT);

    LOGFONT lf;
    GetObject(defaultFont, sizeof(LOGFONT), &lf);
    lf.lfWeight = FW_BOLD;

    HFONT boldFont = CreateFontIndirect(&lf);
    SetProp(hwnd, BOLDFONTPROP, (HANDLE)boldFont);
}
void BoldFont_Delete(HWND hwnd)
{
    HFONT boldFont = (HFONT)RemoveProp(hwnd, BOLDFONTPROP );
    DeleteObject(boldFont);
}
HFONT BoldFont_Get(HWND hwnd)
{
    return (HFONT)GetProp(hwnd, BOLDFONTPROP);
}

void Windows_Create(HWND hwnd)
{
    HWND hwndMain = GetMainWindow(hwnd);
    WindowsVector *windows = new WindowsVector;
    SetProp(hwndMain, WINDOWSPROP, (HANDLE)windows);
}
WindowsVector *Windows_Get(HWND hwnd)
{
    HWND hwndMain = GetMainWindow(hwnd);
    return (WindowsVector*)GetProp(hwndMain, WINDOWSPROP);
}
void Windows_Destroy(HWND hwnd)
{
    WindowsVector *windowsVec = Windows_Get(hwnd);
    delete windowsVec;
}

tstring Version_ToString(const VERSION& version)
{
    switch(version.state)
    {
        case VS_FILENOTFOUND: return _T("Not installed"); break;
        case VS_NOVERSIONINFO: return _T("No version found"); break;
        default: return strprintf(_T("%d.%d.%d"), version.major, version.minor, version.release);
    }
}
