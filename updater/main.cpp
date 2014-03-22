#include <windows.h>
#include <string>
#include <tchar.h>
#include "resource.h"

#define WM_UPDATERTHREAD_END    (WM_APP+1)
typedef std::basic_string<TCHAR> tstring;

tstring Updater_GetWorkingFolder()
{
    TCHAR tpath[MAX_PATH] = _T("");
    GetModuleFileName(NULL, tpath, MAX_PATH);
    
    tstring path = tpath;
    return path.substr( 0, 1+path.rfind(_T('\\')) );
}
int Updater_UpdateFiles(const tstring& folder)
{
    tstring pattern = folder + _T("*.*");
    int fileUpdatedCount = 0;
    
    WIN32_FIND_DATA wfd = {0};
    HANDLE file = FindFirstFile(pattern.c_str(), &wfd);
    
    do 
    {
        if (file && wfd.cFileName[0] != '.')
        {
            if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                //Search in subdirectories
                tstring subFolder = folder + wfd.cFileName + _T("\\");
                fileUpdatedCount += Updater_UpdateFiles(subFolder);
            }
            else
            {
                //Rename files
                tstring filePath = folder + wfd.cFileName;
                tstring extension = filePath.substr(filePath.rfind(_T('.')) +1);
                
                if (extension == _T("update"))
                {
                    tstring newFilePath = filePath.substr(0, filePath.rfind(_T('.')));
                    
                    //Copy the update file over the old file
                    CopyFile(filePath.c_str(), newFilePath.c_str(), FALSE);
                    
                    //Remove the update file
                    DeleteFile(filePath.c_str());
                    
                    fileUpdatedCount++;
                }
            }
        }
    } while (FindNextFile(file, &wfd));
    
    FindClose(file);
    return fileUpdatedCount;
}
DWORD WINAPI Updater_ThreadProc(LPVOID param)
{
    HWND hwnd = (HWND)param;
    tstring workingFolder = Updater_GetWorkingFolder();
    int fileUpdatedCount = Updater_UpdateFiles(workingFolder);
    
    if (fileUpdatedCount)
    {
        //TodoManager might still be running...
        Sleep(2000);
        
        tstring tePath = workingFolder + _T("TrayEverything.exe");
        ShellExecute(NULL, NULL, tePath.c_str(), NULL, NULL, SW_SHOW);
    }
    
    PostMessage(hwnd, WM_UPDATERTHREAD_END, 0, 0);
    return 0;
}

BOOL CALLBACK UpdaterDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE thread = NULL;
    static HICON mainIcon = NULL;
    
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            mainIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_UPDATER));
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)mainIcon);
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)mainIcon);
            SetWindowText(hwnd, _T("Updating TrayEverything..."));
            
            SendDlgItemMessage(hwnd, IDC_UPDATERICON, STM_SETICON, (WPARAM)mainIcon, 0);
            
            DWORD threadId = 0;
            thread = CreateThread(0,0, Updater_ThreadProc, hwnd, 0, &threadId);
        }
        break;
        
        case WM_CLOSE:
        {
            DestroyIcon(mainIcon);
            EndDialog(hwnd, 0);
        }
        break;
        
        case WM_UPDATERTHREAD_END:
        {
            WaitForSingleObject(thread, 1000);
            CloseHandle(thread);
            
            PostMessage(hwnd, WM_CLOSE, 0,0);
        }
        break;
    }
    
    return FALSE;
}

int WINAPI WinMain (HINSTANCE instance, HINSTANCE prevInstance,LPSTR args,int showCmd)
{
    return DialogBox(instance, MAKEINTRESOURCE(IDD_UPDATERDLG), NULL, UpdaterDlgProc);
}
