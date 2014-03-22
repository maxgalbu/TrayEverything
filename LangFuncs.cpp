#include "resource.h"
#include "CommonFuncs.h"
#include "OptionsFuncs.h"

BOOL GetNextLanguageDll(HANDLE& fileHandle, LPTSTR filePath)
{
    WIN32_FIND_DATA wfd;
    tstring langDir = GetProgramDir() + _T("lang\\");
    
    if (!fileHandle)
    {
        fileHandle = FindFirstFile( (langDir+ _T("*.dll") ).c_str(), &wfd);
        if (fileHandle == INVALID_HANDLE_VALUE)
            return FALSE;
    }
    else
    {
        if (!FindNextFile(fileHandle, &wfd))
        {
            FindClose(fileHandle);
            return FALSE;
        }
    }
    
    _stprintf(filePath, _T("%s%s"), langDir.c_str(), wfd.cFileName);
    return TRUE;
}

BOOL Lang_Load(HWND hwnd, BOOL recover)
{
    HWND hwndMain = GetMainWindow(hwnd);
    tstring lang = Options_GetString(hwnd, LANGUAGE);
    
    TCHAR langDllPath[1024] = _T("");
    HANDLE langDllFile = NULL;
    BOOL found = FALSE;
    
    while(GetNextLanguageDll(langDllFile, langDllPath) != FALSE)
    {
        HMODULE langDll = LoadLibrary(langDllPath);
        if (langDll)
        {
            TCHAR shortName[10] = _T("");
            LoadString(langDll, IDS_SHORTNAME, shortName, 10);
            
            if (shortName == lang)
            {
                found = TRUE;
                SetProp(hwndMain, LANGUAGEPROP, (HANDLE)langDll);
            }
            else
                FreeLibrary(langDll);
        }
    }
    
    if (found)
        return TRUE;
    
    if (!recover)
    {
        //Still here, there's an error
        MessageBox(hwnd, _T("The language dll cannot be found. TrayEverything will ")
            _T("now restore the default language (english) and will exit."), _T("TrayEverything"), MB_OK|MB_TOPMOST);
        Options_SetString(hwnd, LANGUAGE, _T("en"));
        
        if (!Lang_Load(hwnd, TRUE))
            DestroyWindow(hwndMain);
    }
    
    return FALSE;
}
void Lang_Unload(HWND hwnd)
{
    HWND hwndMain = GetMainWindow(hwnd);
    HMODULE langDll = (HMODULE)RemoveProp(hwndMain, LANGUAGEPROP);
    FreeLibrary(langDll);
}

tstring Lang_LoadString(HWND hwnd, UINT stringId)
{
    if (hwnd)
    {
        HWND hwndMain = GetMainWindow(hwnd);
        HMODULE langDll = (HMODULE)GetProp(hwndMain, LANGUAGEPROP);
        
        TCHAR langString[512] = _T("");
        LoadString(langDll, stringId, langString, 512);
        return langString;
    }
    
    return _T("");
}
