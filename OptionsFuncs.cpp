#include "resource.h"
#include "CommonFuncs.h"
#include "HookFuncs.h"
#include "IniParser.h"
#include "LangFuncs.h"
#include "MainFuncs.h"
#include "StringConversions.h"
#include "UpdateDlg.h"

#include "OptionsFuncs.h"

void Options_Load(HWND hwnd)
{
    HWND hwndMain = GetMainWindow(hwnd);
    
    OPTIONS *opts = new OPTIONS;
    SetProp(hwndMain, OPTIONSPROP, (HANDLE)opts);
    
    IniParser ini;
    ini.FromFile( GetProgramDir() + _T("options.ini") );
    
    opts->intValues[WL_ORDER_INDEX] =           ini.GetInt(_T("WindowsList"), _T("OrderIndex"), -1);
    opts->intValues[WL_ORDER_ASCENDING] =       ini.GetInt(_T("WindowsList"), _T("OrderAscending"), FALSE);
    
    opts->stringValues[LANGUAGE] =              ini.GetString(_T("General"), _T("Language"), _T("en"));
    opts->intValues[KBHOTKEY] =                 ini.GetInt(_T("General"), _T("KeyboardHotkey"), KBH_NONE);
    opts->intValues[STARTUP] =                  ini.GetInt(_T("General"), _T("StartAtStartup"), FALSE);
    
    opts->intValues[MAINTRAY_MINONCLOSE] =      ini.GetInt(_T("MainTray"), _T("MinOnClose"), TRUE);
    opts->intValues[MAINTRAY_HIDEONMIN] =       ini.GetInt(_T("MainTray"), _T("HideOnMinimize"), FALSE);
    
    opts->intValues[TRAY_NOICON] =              ini.GetInt(_T("Tray"), _T("NoIcon"), FALSE);
    opts->intValues[TRAY_PWDPROTECT] =          ini.GetInt(_T("Tray"), _T("PwdProtection"), FALSE);
    opts->intValues[TRAY_GROUPICON] =           ini.GetInt(_T("Tray"), _T("GroupIcon"), FALSE);
    opts->intValues[TRAY_HOOK] =                ini.GetInt(_T("Tray"), _T("Hook"), HO_NONE);
    opts->intValues[TRAY_AUTOMIN] =             ini.GetInt(_T("Tray"), _T("AutoMinimize"), FALSE);
    opts->intValues[TRAY_AUTOMIN_TIME] =        ini.GetInt(_T("Tray"), _T("AutoMinimizeTime"), 0);
    opts->intValues[TRAY_AUTOMIN_TIMETYPE] =    ini.GetInt(_T("Tray"), _T("AutoMinimizeTimeType"), TT_SECONDS);
    
    //Get the list of programs (stored as keys under the ProgramsList group)
    StringVector programsKeys = ini.GetGroupKeys(_T("ProgramsList"));
    for (int i = 0; i < programsKeys.size(); i++)
    {
        tstring programPath = programsKeys[i];
        tstring programOptions = ini.GetString(_T("ProgramsList"), programPath.c_str(), _T("0,0,0,0,0"));
        
        //Get the options for this program
        StringVector programOptionsStringVector = explode(programOptions, _T(","));
        
        //If there arent' five options, fill the vector
        //until there are five options
        while (programOptionsStringVector.size() != 5)
            programOptionsStringVector.push_back( _T("0") );
        
        //Convert string values to int values
        IntVector *programOptionsVector = new IntVector;
        for (int i = 0; i < programOptionsStringVector.size(); i++)
            programOptionsVector->push_back( a2i(programOptionsStringVector[i]) );
        
        opts->programPathsOptionsValues[ programPath ] = programOptionsVector;
    }
    
    opts->intValues[UPDATES_CHECK] =            ini.GetInt(_T("Updates"), _T("Check"), UG_NONE);
    opts->intValues[UPDATES_LAST_DATE] =        ini.GetInt(_T("Updates"), _T("LastDate"), 0);
}
void Options_Save(HWND hwnd)
{
    HWND hwndMain = GetMainWindow(hwnd);
    OPTIONS *opts = (OPTIONS*)GetProp(hwndMain, OPTIONSPROP);
    if (!opts) return;
    
    IniParser ini;
    ini.FromFile( GetProgramDir() + _T("options.ini") );
    
    ini.SetInt(_T("WindowsList"), _T("OrderIndex"), opts->intValues[WL_ORDER_INDEX]);
    ini.SetInt(_T("WindowsList"), _T("OrderAscending"), opts->intValues[WL_ORDER_ASCENDING]);
    
    ini.SetString(_T("General"), _T("Language"), opts->stringValues[LANGUAGE]);
    ini.SetInt(_T("General"), _T("KeyboardHotkey"), opts->intValues[KBHOTKEY]);
    ini.SetInt(_T("General"), _T("StartAtStartup"), opts->intValues[STARTUP]);
    
    ini.SetInt(_T("MainTray"), _T("MinOnClose"), opts->intValues[MAINTRAY_MINONCLOSE]);
    ini.SetInt(_T("MainTray"), _T("HideOnMinimize"), opts->intValues[MAINTRAY_HIDEONMIN]);
    
    ini.SetInt(_T("Tray"), _T("NoIcon"), opts->intValues[TRAY_NOICON]);
    ini.SetInt(_T("Tray"), _T("PwdProtection"), opts->intValues[TRAY_PWDPROTECT]);
    ini.SetInt(_T("Tray"), _T("GroupIcon"), opts->intValues[TRAY_GROUPICON]);
    ini.SetInt(_T("Tray"), _T("Hook"), opts->intValues[TRAY_HOOK]);
    ini.SetInt(_T("Tray"), _T("AutoMinimize"), opts->intValues[TRAY_AUTOMIN]);
    ini.SetInt(_T("Tray"), _T("AutoMinimizeTime"), opts->intValues[TRAY_AUTOMIN_TIME]);
    ini.SetInt(_T("Tray"), _T("AutoMinimizeTimeType"), opts->intValues[TRAY_AUTOMIN_TIMETYPE]);
    
    std::map<tstring, IntVector*>:: iterator iter = opts->programPathsOptionsValues.begin();
    for ( ; iter != opts->programPathsOptionsValues.end(); iter++)
    {
        tstring options = _T("");
        for (int i = 0; i < iter->second->size(); i++)
        {
            if (options != _T(""))
                options += _T(",");
            options += i2a( iter->second->at(i) );
        }
        ini.SetString(_T("ProgramsList"), iter->first.c_str(), options);
    }
    
    ini.SetInt(_T("Updates"), _T("Check"), opts->intValues[UPDATES_CHECK]);
    ini.SetInt(_T("Updates"), _T("LastDate"), opts->intValues[UPDATES_LAST_DATE]);
    
    //Erase the old key which holded the programs to be trayed 
    //when clicked on the minimize button
    ini.EraseKey(_T("Tray"), _T("HookMinProgrs"));
    ini.EraseKey(_T("Tray"), _T("HookMinChoose"));
    
    ini.ToFile( GetProgramDir() + _T("options.ini") );
}
void Options_Destroy(HWND hwnd)
{
    HWND hwndMain = GetMainWindow(hwnd);
    OPTIONS *options = (OPTIONS*)RemoveProp(hwndMain, OPTIONSPROP);
    
    std::map<tstring, IntVector*>::iterator iter = options->programPathsOptionsValues.begin();
    for (; iter != options->programPathsOptionsValues.end(); iter++)
        delete (iter->second);
    
    delete options;
}
void Options_Restore(HWND hwnd)
{
    int kbHotkey = Options_GetInt(hwnd, KBHOTKEY);
    switch (kbHotkey)
    {
        default:
        case KBH_NONE:
            break;
        
        case KBH_F7:
            RegisterHotKey(hwnd, MINHOTKEY_ID, 0, VK_F7);
            break;
        case KBH_WINQ:
            RegisterHotKey(hwnd, MINHOTKEY_ID, MOD_WIN, 'Q');
            break;
        case KBH_WINS:
            RegisterHotKey(hwnd, MINHOTKEY_ID, MOD_WIN, 'S');
            break;
        case KBH_CTRLQ:
            RegisterHotKey(hwnd, MINHOTKEY_ID, MOD_CONTROL, 'Q');
            break;
    }
    
    BOOL update = FALSE;
    switch(Options_GetInt(hwnd, UPDATES_CHECK))
    {
        case UG_EVERYSTART:
            update = TRUE;
            break;
        
        case UG_EVERYDAY:
        {
            // Se è passato un giorno
            if (IsDayGone(hwnd))
                update = TRUE;
        }
        break;
        
        case UG_EVERYWEEK:
        {
            // Se è passato una settimana
            if (IsWeekGone(hwnd))
                update = TRUE;
        }
        break;
    }
    
    if (update)
    {
        TBBUTTONINFO tbi = {0};
        tbi.cbSize = sizeof(TBBUTTONINFO);
        tbi.dwMask = TBIF_TEXT;
        tbi.pszText = (LPTSTR)Lang_LoadString(hwnd, IDS_TB_CHECKINGUPD).c_str();
        
        HWND toolbar = GetDlgItem(hwnd, IDC_TOOLBAR);
        ToolBar_SetButtonInfo(toolbar, IDC_TB_UPDATE, &tbi);
        
        Updates_Check(hwnd, TRUE);
    }
    
    //Set the timer to check for active windows
    SetTimer(hwnd, ACTIVEWINDOWS_TIMER_ID, 1000, NULL);
    
    if (Options_GetInt(hwnd, TRAY_HOOK) != HO_NONE)
        Hook_Start();
}

int Options_GetInt(HWND hwnd, int option)
{
    HWND hwndMain = GetMainWindow(hwnd);
    OPTIONS *opts = (OPTIONS*)GetProp(hwndMain, OPTIONSPROP);
    if (!opts)
    {
        MessageBox(hwnd, _T("Can't find the options."), NULL, 0);
        return 0;
    }
    
    std::map<int,int>::iterator findIter = opts->intValues.find(option);
    if (findIter != opts->intValues.end())
        return findIter->second;
    else
    {
        MessageBox(hwnd, _T("Can't find the requested option."), NULL, 0);
        return 0;
    }
}
tstring Options_GetString(HWND hwnd, int option)
{
    HWND hwndMain = GetMainWindow(hwnd);
    OPTIONS *opts = (OPTIONS*)GetProp(hwndMain, OPTIONSPROP);
    if (!opts)
    {
        MessageBox(hwnd, _T("Can't find the options."), NULL, 0);
        return _T("");
    }
    
    std::map<int,tstring>::iterator findIter = opts->stringValues.find(option);
    if (findIter != opts->stringValues.end())
        return findIter->second;
    else
    {
        MessageBox(hwnd, _T("Can't find the requested option."), NULL, 0);
        return _T("");
    }
}
IntVector *Options_GetProgramOptionsVector(HWND hwnd, tstring program)
{
    HWND hwndMain = GetMainWindow(hwnd);
    OPTIONS *opts = (OPTIONS*)GetProp(hwndMain, OPTIONSPROP);
    if (!opts)
    {
        MessageBox(hwnd, _T("Can't find the options."), NULL, 0);
        return NULL;
    }
    
    //To lower case
    std::transform(program.begin(), program.end(), program.begin(), tolower);
    
    std::map<tstring, IntVector*>::iterator findIter = opts->programPathsOptionsValues.find(program);
    if (findIter != opts->programPathsOptionsValues.end())
        return findIter->second;
    else
    {
        IntVector *programOptionsVector = new IntVector;
        while (programOptionsVector->size() != 5)
            programOptionsVector->push_back(0);
        
        opts->programPathsOptionsValues[program] = programOptionsVector;
        return programOptionsVector;
    }
}
StringVector Options_GetProgramPathsVector(HWND hwnd)
{
    StringVector programPathsVector;
    
    HWND hwndMain = GetMainWindow(hwnd);
    OPTIONS *opts = (OPTIONS*)GetProp(hwndMain, OPTIONSPROP);
    if (!opts)
    {
        MessageBox(hwnd, _T("Can't find the options."), NULL, 0);
        return programPathsVector;
    }
    
    std::map<tstring, IntVector*>::iterator iter = opts->programPathsOptionsValues.begin();
    for ( ; iter != opts->programPathsOptionsValues.end(); iter++)
        programPathsVector.push_back(iter->first);
    
    return programPathsVector;
}

void Options_SetInt(HWND hwnd, int option, int value)
{
    HWND hwndMain = GetMainWindow(hwnd);
    OPTIONS *opts = (OPTIONS*)GetProp(hwndMain, OPTIONSPROP);
    if (!opts)
    {
        MessageBox(hwnd, _T("Can't find the options."), NULL, 0);
        return;
    }
    
    std::map<int,int>::iterator findIter = opts->intValues.find(option);
    if (findIter != opts->intValues.end())
        findIter->second = value;
    else
    {
        MessageBox(hwnd, _T("Can't find the requested option."), NULL, 0);
        return;
    }
}
void Options_SetString(HWND hwnd, int option, tstring value)
{
    HWND hwndMain = GetMainWindow(hwnd);
    OPTIONS *opts = (OPTIONS*)GetProp(hwndMain, OPTIONSPROP);
    if (!opts)
    {
        MessageBox(hwnd, _T("Can't find the options."), NULL, 0);
        return;
    }
    
    std::map<int,tstring>::iterator findIter = opts->stringValues.find(option);
    if (findIter != opts->stringValues.end())
        findIter->second = value;
    else
    {
        MessageBox(hwnd, _T("Can't find the requested option."), NULL, 0);
        return;
    }
}
void Options_SetProgramOptionsVector(HWND hwnd, tstring program, IntVector *options)
{
    HWND hwndMain = GetMainWindow(hwnd);
    OPTIONS *opts = (OPTIONS*)GetProp(hwndMain, OPTIONSPROP);
    if (!opts)
    {
        MessageBox(hwnd, _T("Can't find the options."), NULL, 0);
        return;
    }
    
    std::map<tstring, IntVector*>::iterator findIter = opts->programPathsOptionsValues.find(program);
    if (findIter != opts->programPathsOptionsValues.end())
    {
        delete findIter->second;
        IntVector *newOptions = new IntVector;
        *newOptions = *options;
        findIter->second = newOptions;
    }
    else
    {
        MessageBox(hwnd, _T("Can't find the requested program."), NULL, 0);
        return;
    }
}
