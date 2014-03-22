#define _WIN32_IE	    0x601
#define _WIN32_WINNT	0x601

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>
#include <tlhelp32.h>

#include <uxtheme.h>
#include <tmschema.h>

#include <stdio.h>
#include <time.h>

#ifndef COMPILING_RES_FILE
    #include <string>
    #include <vector>
    #include <map>
    #include <algorithm>
#endif

#include "commctrl_macros.h"
#include "lang/lang.h"

#ifndef HDF_SORTUP
    #define HDF_SORTUP 0x0400
#endif
#ifndef HDF_SORTDOWN
    #define HDF_SORTDOWN 0x0200
#endif
#ifndef TBCDRF_USECDCOLORS
    #define TBCDRF_USECDCOLORS 0x00800000
#endif
#ifndef LVN_HOTTRACK
    #define LVN_HOTTRACK (LVN_FIRST-21)
#endif
#ifndef DTT_GLOWSIZE
    #define DTT_GLOWSIZE (1UL << 11)
#endif
#ifndef DTT_COMPOSITED
    #define DTT_COMPOSITED (1UL << 13)
#endif
#ifndef WM_DWMCOMPOSITIONCHANGED
    #define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif

#define WM_LANGUAGECHANGED          (WM_APP+1)
#define WM_MAINTRAY_NOTIFY          (WM_APP+2)
#define WM_PROGTRAY_NOTIFY          (WM_APP+3)
#define WM_CHECKUPDTHREAD_END       (WM_APP+4)
#define WM_DOWNLOADUPDTHREAD_END    (WM_APP+5)
#define WM_TABKEYDOWN               (WM_APP+6)
#define WINDOWSPROP                 _T("TrayWindows")
#define MAINTRAYMENUPROP            _T("TrayMenu")
#define DIALOGSPROP                 _T("Dialogs")
#define CURRENTDLGPROP              _T("CurrentDialog")
#define ORIGINALPROCPROP            _T("OriginalProc")
#define LANGUAGEPROP                _T("Language")
#define OPTIONSPROP                 _T("Options")
#define UPDATESPROP                 _T("Updates")
#define CHECKUPDTHREADPROP          _T("CheckForUpdatesThread")
#define BOLDFONTPROP                _T("BoldFont")
#define DEFAULTFONTPROP             _T("DefaultFont")
#define SILENTUPDATEPROP            _T("SilentUpdate")
#define PROGRAMOPTION_ALWAYSMINIMIZE    0
#define PROGRAMOPTION_NEVERMINIMIZE     1
#define PROGRAMOPTION_NOAUTOMINIMIZE    2
#define PROGRAMOPTION_NOTRAYBUTTON      3
#define PROGRAMOPTION_NOHOOKMINBUTTON   4

#define WM_TE_HOOKED                _T("TrayEverythingHookedMsg")
#define WM_TE_UNHOOKED              _T("TrayEverythingUnhookedMsg")
#define WM_TE_MINIMIZE              _T("TrayEverythingMinimizeMsg")
#define WM_TE_GETHOOKOPTIONS        _T("TrayEverythingGetHookOptionsMsg")
#define WM_TE_SETHOOKOPTIONS        _T("TrayEverythingSetHookOptionsMsg")
#define WM_TE_UPDATEHOOKOPTIONS     _T("TrayEverythingUpdateHookOptionsMsg")

#define IDI_MAINICON                1
#define IDB_TRAY                    10
#define IDB_SETTINGS                11
#define IDB_MINIMIZE                12
#define IDB_UPDATE                  13
#define IDB_TOPMOST                 14
#define IDB_ABOUT                   15
#define IDB_EXIT                    16
#define IDB_ARROWUP                 17
#define IDB_ARROWDOWN               18
#define IDB_SPLASHSCREEN            19
#define IDI_EXPAND                  20
#define IDI_COLLAPSE                21

#define MAINTRAY_ID                 3001
#define MINHOTKEY_ID                3002
#define TEHOTKEY_ID                 3003

#define OPENWINDOWS_TIMER_ID        4001
#define ACTIVEWINDOWS_TIMER_ID      4002

#define IDD_MAINDLG			        9001
#define IDC_REBAR                   101
#define IDC_TOOLBAR                 102
#define IDC_WINDOWSLIST             103
#define IDC_STATUSBAR               104

#define IDMC_PROGTRAY_RESTORE       101

#define IDMC_MAINTRAY_RESTORE       301
#define IDMC_MAINTRAY_SETTINGS      302
#define IDMC_MAINTRAY_UPDATE        303
#define IDMC_MAINTRAY_ABOUT         304
#define IDMC_MAINTRAY_EXIT          305

//Listview menu
#define IDMC_WL_MENU_MINIMIZE       401
#define IDMC_WL_MENU_RESTORE        402
#define IDMC_WL_MENU_PROPS          403

//System menu and listview menu items
#define IDMC_LS_MENU_MINTOTRAY      501

//Main toolbar
#define IDC_TB_TRAY                 201
#define IDC_TB_SETTINGS             202
#define IDC_TB_MINIMIZE             203
#define IDC_TB_UPDATE               204
#define IDC_TB_TOPMOST              205
#define IDC_TB_ABOUT                206
#define IDC_TB_EXIT     		    207

#define IDD_OPTIONSDLG              9002
#define IDC_OPTS_TAB                101
#define IDC_OPTS_APPLY              102
#define IDC_OPTS_DEFAULT            103

#define IDD_OPTS_GENDLG             9003
#define IDC_GOD_WINDOWS_RUN         201
#define IDC_GOD_UPD_CHECK           202
#define IDC_GOD_UPD_EVERYSTART      203
#define IDC_GOD_UPD_EVERYDAY        204
#define IDC_GOD_UPD_EVERYWEEK       205
#define IDH_GOD_LANGUAGE            206
#define IDC_GOD_LANGUAGE            207

#define IDD_OPTS_TRAYDLG            9004
#define IDH_TOD_MAIN                301
#define IDC_TOD_TE_MINONCLOSE       302
#define IDC_TOD_TE_HIDEONMIN        303
#define IDH_TOD_TE_HIDEONMIN        304
#define IDH_TOD_OTHERPROGS          305
#define IDC_TOD_OP_NOICON           306
#define IDC_TOD_OP_PWDPROTECT       307
#define IDC_TOD_OP_GROUPICON        308

#define IDD_OPTS_TRAY2DLG           9005
#define IDH_T2OD_AUTOMINIMIZE       401
#define IDC_T2OD_AUTOMINIMIZE       402
#define IDC_T2OD_AM_TIME            403
#define IDC_T2OD_AM_TIMETYPE        404
#define IDH_T2OD_MINFROMPROGR       405
#define IDC_T2OD_MFP_NONE           406
#define IDC_T2OD_MFP_TRAYBUTTON     407
#define IDC_T2OD_MFP_MINBUTTON      408
#define IDC_T2OD_MFP_MIDDLECLICK    409

#define IDD_OPTS_PROGRAMSDLG        9006
#define IDC_POD_PROGRAMSLIST        501

#define IDD_OPTS_HOTKEYSDLG         9007
#define IDC_HOD_KBH_ENABLE          601
#define IDC_HOD_KBH_F7              602
#define IDC_HOD_KBH_WINQ            603
#define IDC_HOD_KBH_WINS            604
#define IDC_HOD_KBH_CTRLQ           605

#define IDD_SETPASSWORDDLG          9008
#define IDH_SP_PWD1                 101
#define IDC_SP_PWD1                 102
#define IDH_SP_PWD2                 103
#define IDC_SP_PWD2                 104

#define IDD_GETPASSWORDDLG          9009
#define IDH_GP_PWD                  101
#define IDC_GP_PWD                  102
#define IDC_GP_STATUS               103

#define IDD_UPDATEDLG               9010
#define IDC_UD_UPDATESLIST          801
#define IDC_UD_PROGRESS             802
#define IDH_UD_PERCENT              803
#define IDC_UD_PERCENT              804
#define IDH_UD_STATUS               805
#define IDC_UD_STATUS               806
#define IDH_UD_COMPLETED            807
#define IDC_UD_COMPLETED            808
#define IDC_UD_CHECK                809
#define IDC_UD_DOWNLOAD             810

#define IDD_ABOUTDLG                9011
#define IDC_AD_HEADER               101
#define IDC_AD_ICON                 102
#define IDH_AD_VERSION              103
#define IDC_AD_VERSION              104
#define IDH_AD_CREDITS              105
#define IDC_AD_CREDITS              106
#define IDC_AD_WEB                  107
#define IDC_AD_CHECKUPDATES         108

extern UINT externMinimizeMsg;
extern UINT taskbarCreatedMsg;
extern UINT getHookOptionsMsg;
extern UINT setHookOptionsMsg;
extern UINT updateHookOptionsMsg;

enum options {
    WL_ORDER_INDEX, WL_ORDER_ASCENDING,
    STARTUP, LANGUAGE, UPDATES_CHECK, UPDATES_LAST_DATE,
    MAINTRAY_MINONCLOSE, MAINTRAY_HIDEONMIN, TRAY_NOICON, TRAY_PWDPROTECT, TRAY_GROUPICON,
        TRAY_HOOK, TRAY_AUTOMIN, TRAY_AUTOMIN_TIME, TRAY_AUTOMIN_TIMETYPE,
    KBHOTKEY
};
enum kbHotkey {
    KBH_NONE, KBH_F7, KBH_WINQ, KBH_WINS, KBH_CTRLQ
};
enum updateGap {
    UG_NONE, UG_EVERYSTART, UG_EVERYDAY, UG_EVERYWEEK
};
enum verState {
    VS_NONE, VS_FILENOTFOUND, VS_NOVERSIONINFO
};
enum whatToUpdate {
    WTU_NONE, WTU_WINDOWTEXT, WTU_VISIBLE
};
enum timeType {
    TT_SECONDS, TT_MINUTES
};
enum hookOption {
    HO_NONE, HO_TRAYBUTTON, HO_MINBUTTON, HO_MIDDLECLICK
};

typedef std::basic_string<TCHAR> tstring;
typedef std::vector<tstring> StringVector;
typedef std::vector<int> IntVector;

typedef struct tagOPTIONS
{
    std::map<int,int> intValues;
    std::map<int,tstring> stringValues;
    std::map<tstring, IntVector*> programPathsOptionsValues;
} OPTIONS;
typedef struct tagWINDOW
{
    tagWINDOW() : uniqueId(0), uniqueGroupId(0), imageIndex(0), hwnd(NULL), processID(0), path(_T("")),
        filename(_T("")), windowText(_T("")), isMinimized(FALSE), trayMenu(NULL), password(_T("")),
        toAdd(TRUE), toUpdate(WTU_NONE), toRemove(FALSE), secondsInactive(0)
    { }

    unsigned long uniqueId;
    unsigned long uniqueGroupId;

    int imageIndex;
    HWND hwnd;
    DWORD processID;
    tstring path;
    tstring filename;
    tstring windowText;
    BOOL isMinimized;
    HMENU trayMenu;
    tstring password;

    BOOL toAdd;
    int toUpdate;
    BOOL toRemove;

    int secondsInactive;
} WINDOW;
typedef struct tagVERSION
{
    int state;
    int major;
    int minor;
    int release;
} VERSION;
typedef struct tagUPDATE
{
    tstring name;
    tstring from;
    tstring to;
    DWORD size;
    VERSION updateVersion;
    VERSION installedVersion;
    BOOL requiresRestart;
    //IntVector dependencies;
    tstring notes;

    BOOL successful;
} UPDATE;
typedef struct tagMINIMIZERESTORECHILDWINDOWSSTRUCT
{
    DWORD processID;
    BOOL restore;
} MINIMIZERESTORECHILDWINDOWSSTRUCT;

typedef std::vector<WINDOW> WindowsVector;
typedef std::vector<UPDATE> UpdatesVector;
typedef std::map<int,HWND> DialogMap;

typedef HRESULT (WINAPI *DWMISCOMPOSITIONENABLED)(BOOL* pfEnabled);
typedef HRESULT (WINAPI *DWMEXTENDFRAMEINTOCLIENTAREA)(HWND hWnd, const MARGINS* pMarInset);
