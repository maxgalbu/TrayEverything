#include "resource.h"
#include "CommonFuncs.h"

HINSTANCE hookDll = NULL;
HHOOK hook = NULL;

void Hook_Start()
{
    if (hook)
    {
        //Request other applications to update their options
        UINT updateHookOptionsMsg = RegisterWindowMessage(WM_TE_UPDATEHOOKOPTIONS);
        SendMessage(HWND_BROADCAST, updateHookOptionsMsg, 0,0);
    }
    else
    {
        //Set the hook
        hookDll = LoadLibrary(_T("hook.dll"));
        HOOKPROC hookProc = (HOOKPROC)GetProcAddress(hookDll, "GetMessageProc@12");
        
        hook = SetWindowsHookEx(WH_GETMESSAGE, hookProc, hookDll, 0);
        if (!hook)
            ErrorBox(NULL, FALSE, _T("Unable to set the hook. Error: %d"), GetLastError());
    }
}
void Hook_Stop()
{
    if (hook)
    {
        //Broadcast a "unhooked" message
        UINT unhookMsg = RegisterWindowMessage(WM_TE_UNHOOKED);
        SendMessage(HWND_BROADCAST, unhookMsg, 0,0);
        
        /*DWORD recipients = BSM_APPLICATIONS;
        BroadcastSystemMessage(BSF_FLUSHDISK|BSF_FORCEIFHUNG|BSF_IGNORECURRENTTASK, &recipients, unhookMsg, 0,0);*/
        
        //Unset the hook
        BOOL ret = UnhookWindowsHookEx(hook);
        if (ret == FALSE)
            ErrorBox(NULL, FALSE, _T("Unable to unset the hook. Error: %d"), GetLastError());
        
        FreeLibrary(hookDll);
        hook = NULL;
    }
}
