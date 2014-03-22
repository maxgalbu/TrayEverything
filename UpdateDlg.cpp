#include "resource.h"
#include <shlwapi.h>
#include <wininet.h>
#include "CommonFuncs.h"
#include "IniParser.h"
#include "LangFuncs.h"
#include "MainFuncs.h"

#include "UpdateDlg.h"


void Updates_GetInstalledVersion(const tstring& fileToUpdate, VERSION *installedVersion)
{
    if (!FileExists(fileToUpdate.c_str()))
    {
        installedVersion->state = VS_FILENOTFOUND;
        return;
    }
    
    //Get version info size
    DWORD fileVersionSize = GetFileVersionInfoSize(fileToUpdate.c_str(), NULL);
    
    if (fileVersionSize == 0)
        installedVersion->state = VS_NOVERSIONINFO;
    //File has version info
    else
    {
        fileVersionSize += 1;
        
        //Get them into a buffer
        BYTE *fileVersion = new BYTE[fileVersionSize];
        GetFileVersionInfo(fileToUpdate.c_str(), 0, fileVersionSize, fileVersion);
        
        //Translate the buffer into a more manageable structure
        VS_FIXEDFILEINFO *ffi = {0};
        VerQueryValue(fileVersion, _T("\\"), (LPVOID*)&ffi, NULL);
        
        //Delete the buffer
        delete [] fileVersion;
        
        installedVersion->major = HIWORD(ffi->dwFileVersionMS);
        installedVersion->minor = LOWORD(ffi->dwFileVersionMS);
        installedVersion->release = HIWORD(ffi->dwFileVersionLS);
    }
}
BOOL Updates_CheckVersions(const VERSION *updateVersion, const VERSION *installedVersion)
{
    if (installedVersion->state == VS_FILENOTFOUND || 
        installedVersion->state == VS_NOVERSIONINFO)
    {
        return TRUE;
    }
    else
    {
        if (updateVersion->major > installedVersion->major)
            return TRUE;
        else if (updateVersion->major == installedVersion->major)
        {
            if (updateVersion->minor > installedVersion->minor)
                return TRUE;
            else if (updateVersion->minor == installedVersion->minor)
            {
                if (updateVersion->release > installedVersion->release)
                    return TRUE;
                else if (updateVersion->release == installedVersion->release)
                    return FALSE;
            }
        }
    }
    
    return FALSE;
}
DWORD Updates_GetSize(const tstring &from)
{
    DWORD fileSize = 0;
    
    HINTERNET session = InternetOpen(_T("TrayEverything Agent"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL,0);
    if (session)
    {
        HINTERNET url = InternetOpenUrl(session, from.c_str(), NULL, 0, INTERNET_FLAG_RELOAD|INTERNET_FLAG_PRAGMA_NOCACHE, 0);
        
        if (url != NULL)
        {
            //Get the status
            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof(DWORD);
            HttpQueryInfo(url, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                (LPVOID)&statusCode, &statusCodeSize, NULL);
            
            if (statusCode == HTTP_STATUS_OK)
            {
                //Get the file size
                DWORD fileSizeSize = sizeof(DWORD);
                HttpQueryInfo(url, HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER, &fileSize, &fileSizeSize, NULL);
                fileSize += 1;
            }
            
            InternetCloseHandle(url);
        }
        
        InternetCloseHandle(session);
    }
    
    return fileSize;
}
DWORD WINAPI Updates_CheckThreadProc(LPVOID param)
{
    HWND hwnd = (HWND)param;
    
    BOOL silent = (BOOL)GetProp(hwnd, SILENTUPDATEPROP);
    
    UpdatesVector *updates = new UpdatesVector;
    Updates_Set(hwnd, updates);
    
    HINTERNET session = InternetOpen(_T("TrayEverything Agent"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL,0);
    HINTERNET url = InternetOpenUrl(session, _T("http://www.winapizone.net/download/software/trayeverything/updates.ini"),
        NULL, 0, INTERNET_FLAG_RELOAD|INTERNET_FLAG_PRAGMA_NOCACHE, 0);
    
    if (url == NULL)
    {
        if (!silent)
            ErrorBox(hwnd, FALSE, IDS_UD_CHK_ERR_FILECONN);
        Updates_Remove(hwnd);
    }
    else
    {
        //Get the status
        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(DWORD);
        HttpQueryInfo(url, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
            (LPVOID)&statusCode, &statusCodeSize, NULL);
        
        if (statusCode != HTTP_STATUS_OK)
        {
            if (!silent)
                ErrorBox(hwnd, FALSE, IDS_UD_CHK_ERR_FILESTATUS, statusCode);
            Updates_Remove(hwnd);
        }
        else
        {
            //Get the file size
            DWORD fileSize = 0;
            DWORD fileSizeSize = sizeof(DWORD);
            HttpQueryInfo(url, HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER, 
                &fileSize, &fileSizeSize, NULL);
            
            fileSize += 1;
            
            //Get the file
            TCHAR *updatesString = new TCHAR[fileSize];
            DWORD bytesRead = 0;
            InternetReadFile(url, updatesString, fileSize, &bytesRead);
            
            //Parse the file
            IniParser ini;
            ini.FromString(updatesString);
            delete [] updatesString;
            
            int updatesCount = ini.GetInt(_T("updates"), _T("count"), 0);
            tstring updatesBase = ini.GetString(_T("updates"), _T("base"), _T(""));
            for(int i = 0; i<updatesCount; i++)
            {
                tstring updateSection = strprintf(_T("update%d"), i);
                
                UPDATE update;
                update.name = ini.GetString(updateSection.c_str(), _T("name"), _T(""));
                update.from = ini.GetString(updateSection.c_str(), _T("from"), _T(""));
                update.to = ini.GetString(updateSection.c_str(), _T("to"), _T(""));
                update.requiresRestart = ini.GetInt(updateSection.c_str(), _T("restart"), 0);
                update.notes = ini.GetString(updateSection.c_str(), _T("notes"), _T(""));
                
                update.from = updatesBase + update.from;
                update.to = GetProgramDir() + update.to;
                
                //Get update size
                update.size = Updates_GetSize(update.from);
                
                //Get update version
                update.updateVersion.major = ini.GetInt(updateSection.c_str(), _T("major"), 0);
                update.updateVersion.minor = ini.GetInt(updateSection.c_str(), _T("minor"), 0);
                update.updateVersion.release = ini.GetInt(updateSection.c_str(), _T("release"), 0);
                
                //Get installed version
                Updates_GetInstalledVersion(update.to, &update.installedVersion);
                
                //Compare the two versions
                if (FALSE == Updates_CheckVersions(&update.updateVersion, &update.installedVersion))
                    continue;
                
                updates->push_back(update);
            }
        }
        
        InternetCloseHandle(url);
        InternetCloseHandle(session);
    }
    
    PostMessage(hwnd, WM_CHECKUPDTHREAD_END, 0,0);
    return 0;
}
void Updates_Check(HWND hwnd, BOOL silent)
{
    DWORD threadId = 0;
    HANDLE checkUpdatesThread = CreateThread(0, 10240000*10, 
        Updates_CheckThreadProc, hwnd, 0, &threadId);
    
    SetProp(hwnd, CHECKUPDTHREADPROP, (HANDLE)checkUpdatesThread);
    SetProp(hwnd, SILENTUPDATEPROP, (HANDLE)silent);
}

void Updates_InitProgress(HWND hwnd, int bytesToRead)
{
    HWND progressBar = GetDlgItem(hwnd, IDC_UD_PROGRESS);
    SendMessage(progressBar, PBM_SETRANGE32, 0, bytesToRead);
}
void Updates_SetProgress(HWND hwnd, int bytesRead)
{
    HWND progressBar = GetDlgItem(hwnd, IDC_UD_PROGRESS);
    SendMessage(progressBar, PBM_SETPOS, bytesRead, 0);
}
void Updates_ClearProgress(HWND hwnd)
{
    HWND progressBar = GetDlgItem(hwnd, IDC_UD_PROGRESS);
    SendMessage(progressBar, PBM_SETPOS, 0, 0);
}
void Updates_SetDownloaded(HWND hwnd, int bytesRead = 0, int bytesToRead = 0)
{
    tstring downloaded = _T("");
    if (bytesRead != 0 && bytesToRead != 0)
    {
        if (bytesToRead == 0)
            downloaded = lstrprintf(hwnd, IDS_UD_PRG_NOSIZE, FormatBytes(bytesRead).c_str());
        else
            downloaded = lstrprintf(hwnd, IDS_UD_PRG, FormatBytes(bytesRead).c_str(), 
                FormatBytes(bytesToRead).c_str());
    }
    
    SetDlgItemText(hwnd, IDC_UD_COMPLETED, downloaded.c_str());
}
void Updates_SetPercent(HWND hwnd, int bytesRead = 0, int bytesToRead = 0)
{
    tstring percent = _T("");
    if (bytesToRead != 0)
        percent = strprintf(_T("%d%%"), (bytesRead*100/bytesToRead));
    
    SetDlgItemText(hwnd, IDC_UD_PERCENT, percent.c_str());
}
void Updates_SetStatus(HWND hwnd, UINT stringId)
{
    Lang_DlgItemText(hwnd, IDC_UD_STATUS, stringId);
}
void Updates_EnableDisableInput(HWND hwnd, BOOL enable)
{
    HMENU systemMenu = GetSystemMenu(hwnd, FALSE);
    UINT flag = (enable ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(systemMenu, MF_BYCOMMAND|flag, SC_CLOSE);
    
    EnableDlgItem(hwnd, IDC_UD_UPDATESLIST, enable);
    EnableDlgItem(hwnd, IDC_UD_CHECK, enable);
    EnableDlgItem(hwnd, IDC_UD_DOWNLOAD, enable);
    
    HWND hwndMain = GetMainWindow(hwnd);
    HWND toolbar = GetDlgItem(hwndMain, IDC_TOOLBAR);
    ToolBar_CheckButton(toolbar, IDC_TB_UPDATE, enable);
    ToolBar_EnableButton(toolbar, IDC_TB_TRAY, enable);
    ToolBar_EnableButton(toolbar, IDC_TB_SETTINGS, enable);
    ToolBar_EnableButton(toolbar, IDC_TB_UPDATE, enable);
    ToolBar_EnableButton(toolbar, IDC_TB_ABOUT, enable);
    ToolBar_EnableButton(toolbar, IDC_TB_EXIT, enable);
}
DWORD WINAPI Updates_DownloadThreadProc(LPVOID param)
{
    HWND hwnd = (HWND)param;   
    UpdatesVector *updates = Updates_Get(hwnd);
    
    Updates_ClearProgress(hwnd);
    Updates_SetDownloaded(hwnd);
    Updates_SetPercent(hwnd);
    
    HINTERNET session = InternetOpen(_T("TrayEverything Agent"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL,0);
    int updatesCount = updates->size();
    for (int i = 0; i<updatesCount; i++)
    {
        HINTERNET url = InternetOpenUrl(session, updates->at(i).from.c_str(),
            NULL, 0, INTERNET_FLAG_RELOAD|INTERNET_FLAG_PRAGMA_NOCACHE, 0);
        
        if (url == NULL)
        {
            ErrorBox(hwnd, FALSE, IDS_UD_ERR_FILECONN, FormatLastInternetError().c_str(),
                updates->at(i).name.c_str());
            Updates_SetStatus(hwnd, IDS_UD_ERRST_FILECONN);
        }
        else
        {
            //Get the status
            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof(DWORD);
            HttpQueryInfo(url, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                (LPVOID)&statusCode, &statusCodeSize, NULL);
            
            if (statusCode != HTTP_STATUS_OK)
            {
                ErrorBox(hwnd, FALSE, IDS_UD_ERR_HTTPERR, statusCode, updates->at(i).name.c_str());
                Updates_SetStatus(hwnd, IDS_UD_ERRST_HTTPERR);
            }
            else
            {
                tstring updateFileName = updates->at(i).to;
                if (updates->at(i).requiresRestart)
                {
                    //When the update needs a restart i can't overwrite it
                    //right now, because probably it's still running.
                    //The updater/renamer will take care of rename the file.
                    updateFileName += _T(".update");
                }
                
                Updates_SetStatus(hwnd, IDS_UD_ST_LOCALFILE);
                HANDLE updateFile = CreateFile(updateFileName.c_str(), GENERIC_WRITE, 0, NULL, 
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                
                if (updateFile == INVALID_HANDLE_VALUE)
                {
                    ErrorBox(hwnd, FALSE, IDS_UD_ERR_LOCALFILE, FormatLastError().c_str(), 
                        updates->at(i).name.c_str());
                    Updates_SetStatus(hwnd, IDS_UD_ERRST_LOCALFILE);
                }
                else
                {
                    //Get the file size
                    DWORD fileSize = 0;
                    DWORD fileSizeSize = sizeof(DWORD);
                    HttpQueryInfo(url, HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER, 
                        &fileSize, &fileSizeSize, NULL);
                    
                    BOOL succedeed = FALSE;
                    BOOL error = FALSE;
                    BYTE buffer[1024];
                    DWORD fileBytesRead = 0;
                    DWORD fileBytesWritten = 0;
                    DWORD fileTotalBytesRead = 0;
                    
                    Updates_SetStatus(hwnd, IDS_UD_ST_DOWNLOAD);
                    Updates_InitProgress(hwnd, fileSize);
                    do
                    {
                        succedeed = InternetReadFile(url, buffer, 
                            sizeof(buffer), &fileBytesRead);
                        
                        if (succedeed)
                        {
                            if (fileBytesRead > 0)
                            {
                                fileTotalBytesRead += fileBytesRead;
                                Updates_SetPercent(hwnd, fileTotalBytesRead, fileSize);
                                Updates_SetDownloaded(hwnd, fileTotalBytesRead, fileSize);
                                Updates_SetProgress(hwnd, fileTotalBytesRead);
                                
                                succedeed = WriteFile(updateFile, buffer, fileBytesRead,
                                    &fileBytesWritten, NULL);
                                
                                if (succedeed && fileBytesWritten != fileBytesRead)
                                    succedeed = FALSE;
                            }
                        }
                        else
                        {
                            ErrorBox(hwnd, FALSE, IDS_UD_ERR_DOWNLOAD, 
                                FormatLastInternetError().c_str(), updates->at(i).name.c_str());
                            Updates_SetStatus(hwnd, IDS_UD_ERRST_DOWNLOAD);
                            
                            error = TRUE;
                            break;
                        }
                    } while (succedeed && fileBytesRead > 0);
                    
                    if (!error)
                        Updates_SetStatus(hwnd, IDS_UD_ST_DONE);
                    
                    updates->at(i).successful = !error;
                    
                    //We're done with this update
                    Updates_ClearProgress(hwnd);
                    CloseHandle(updateFile);
                }
            }
            
            InternetCloseHandle(url);
        }
    }
	
	InternetCloseHandle(session);
	
	PostMessage(hwnd, WM_DOWNLOADUPDTHREAD_END, 0, 0);
    return 0;
}

void UpdatesList_Init(HWND updatesList)
{
    ListView_SetExtendedListViewStyle(updatesList, LVS_EX_FULLROWSELECT);
    
    tstring column1 = Lang_LoadString(updatesList, IDS_UD_UL_COLUMN1);
    tstring column2 = Lang_LoadString(updatesList, IDS_UD_UL_COLUMN2);
    tstring column3 = Lang_LoadString(updatesList, IDS_UD_UL_COLUMN3);
    tstring column4 = Lang_LoadString(updatesList, IDS_UD_UL_COLUMN4);
    
    LVCOLUMN lc = {0};
    lc.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;
    
    lc.cx = 150;
    lc.iSubItem = 0;
    lc.pszText = (LPTSTR)column1.c_str();
    ListView_InsertColumn(updatesList, 0, &lc);
    
    lc.cx = 70;
    lc.iSubItem = 1;
    lc.fmt = LVCFMT_CENTER;
    lc.pszText = (LPTSTR)column2.c_str();
    ListView_InsertColumn(updatesList, 1, &lc);
    
    lc.cx = 70;
    lc.iSubItem = 2;
    lc.pszText = (LPTSTR)column3.c_str();
    ListView_InsertColumn(updatesList, 2, &lc);
    
    lc.cx = 70;
    lc.iSubItem = 3;
    lc.fmt = LVCFMT_RIGHT;
    lc.pszText = (LPTSTR)column4.c_str();
    ListView_InsertColumn(updatesList, 3, &lc);
}
BOOL CALLBACK UpdateDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE thread = NULL;
    
    switch (msg)
    {
        case WM_LBUTTONDOWN:
            MainDlg_MoveOnClick(hwnd);
            break;
        
        case WM_DESTROY:
            Updates_Remove(hwnd);
            break;
        
        case WM_LANGUAGECHANGED:
        {
            Lang_DlgItemText(hwnd, IDH_UD_PERCENT, IDS_UD_PERCENTILE);
            Lang_DlgItemText(hwnd, IDH_UD_STATUS, IDS_UD_STATUS);
            Lang_DlgItemText(hwnd, IDC_UD_STATUS, IDS_UD_ST_READY);
            Lang_DlgItemText(hwnd, IDH_UD_COMPLETED, IDS_UD_COMPLETED);
            Lang_DlgItemText(hwnd, IDC_UD_CHECK, IDS_UD_CHECK);
            Lang_DlgItemText(hwnd, IDC_UD_DOWNLOAD, IDS_UD_DOWNLOAD);
            
            LVCOLUMN lc = {0};
            lc.mask = LVCF_TEXT;
            
            tstring column1 = Lang_LoadString(hwnd, IDS_UD_UL_COLUMN1);
            tstring column2 = Lang_LoadString(hwnd, IDS_UD_UL_COLUMN2);
            tstring column3 = Lang_LoadString(hwnd, IDS_UD_UL_COLUMN3);
            tstring column4 = Lang_LoadString(hwnd, IDS_UD_UL_COLUMN4);
            
            HWND updatesList = GetDlgItem(hwnd, IDC_UD_UPDATESLIST);
            lc.pszText = (LPTSTR)column1.c_str();
            ListView_SetColumn(updatesList, 0, &lc);
            lc.pszText = (LPTSTR)column2.c_str();
            ListView_SetColumn(updatesList, 1, &lc);
            lc.pszText = (LPTSTR)column3.c_str();
            ListView_SetColumn(updatesList, 2, &lc);
            lc.pszText = (LPTSTR)column4.c_str();
            ListView_SetColumn(updatesList, 3, &lc);
        }
        break;
        
        case WM_INITDIALOG:
        {
            HWND updatesList = GetDlgItem(hwnd, IDC_UD_UPDATESLIST);
            UpdatesList_Init(updatesList);
            
            SendMessage(hwnd, WM_LANGUAGECHANGED,0,0);
        }
        break;
        
        case WM_CHECKUPDTHREAD_END:
        {
            RemoveProp(hwnd, SILENTUPDATEPROP);
            HANDLE thread = RemoveProp(hwnd, CHECKUPDTHREADPROP);
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
            
            Updates_EnableDisableInput(hwnd, TRUE);
            
            HWND updatesList = GetDlgItem(hwnd, IDC_UD_UPDATESLIST);
            ListView_DeleteAllItems(updatesList);
            
            UpdatesVector *updates = Updates_Get(hwnd);
            if (!updates)
                EnableDlgItem(hwnd, IDC_UD_DOWNLOAD, FALSE);
            else
            {
                int count = updates->size();
                if (count == 0)
                {
                    InfoBox(hwnd, IDS_UD_MSG_NOUPDATE);
                    EnableDlgItem(hwnd, IDC_UD_DOWNLOAD, FALSE);
                }
                else
                {
                    for (int i = 0; i<count; i++)
                    {
                        int itemIndex = ListView_GetItemCount(updatesList);
                        
                        LVITEM li = {0};
                        li.mask = LVIF_TEXT;
                        li.iItem = itemIndex;
                        
                        li.iSubItem = 0;
                        li.pszText = (LPTSTR)updates->at(i).name.c_str();
                        ListView_InsertItem(updatesList, &li);
                        
                        li.iSubItem = 1;
                        li.pszText = (LPTSTR)Version_ToString(updates->at(i).updateVersion).c_str();
                        ListView_SetItem(updatesList, &li);
                        
                        li.iSubItem = 2;
                        li.pszText = (LPTSTR)Version_ToString(updates->at(i).installedVersion).c_str();
                        ListView_SetItem(updatesList, &li);
                        
                        li.iSubItem = 3;
                        li.pszText = (LPTSTR)FormatBytes(updates->at(i).size).c_str();
                        ListView_SetItem(updatesList, &li);
                    }
                    
                    EnableDlgItem(hwnd, IDC_UD_DOWNLOAD, TRUE);
                }
            }
        }
        break;
        
        case WM_DOWNLOADUPDTHREAD_END:
        {
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
            Updates_EnableDisableInput(hwnd, TRUE);
            
            //Do we need to restart now?
            BOOL restart = FALSE;
            
            //To see if we need it, check all the updates
            UpdatesVector *updates = Updates_Get(hwnd);
            for (int i = 0; i<updates->size(); i++)
            {
                if (updates->at(i).successful && updates->at(i).requiresRestart)
                {
                    restart = TRUE;
                    break;
                }
            }
            
            if (restart)
            {
                //Inform the user
                InfoBox(hwnd, IDS_UD_MSG_RESETTING);
                
                //Close the dialog
                EndDialog(GetMainWindow(hwnd), 0);
                
                //Start the updater
                tstring updaterPath = GetProgramDir() + _T("updater.exe");
                ShellExecute(NULL, NULL, updaterPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
        }
        break;
        
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_UD_CHECK:
                {
                    Updates_EnableDisableInput(hwnd, FALSE);
                    Updates_Check(hwnd);
                }
                break;
                
                case IDC_UD_DOWNLOAD:
                {
                    UpdatesVector *updates = Updates_Get(hwnd);
                    if (!updates)
                        EnableDlgItem(hwnd, IDC_UD_DOWNLOAD, FALSE);
                    else
                    {
                        DWORD threadId = 0;
                        thread = CreateThread(0, 10240000*6, Updates_DownloadThreadProc, hwnd, 
                            0, &threadId);
                        Updates_EnableDisableInput(hwnd, FALSE);
                    }
                }
                break;
            }
        }
        break;
    }
    return FALSE;
}
