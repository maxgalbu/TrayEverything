unsigned long GetUniqueId();

tstring lstrprintf(HWND hwnd, int formatId, ...);
tstring strprintf(LPCTSTR format, ...);
void trim(tstring &str);
StringVector explode(tstring stringToExplode, const tstring& separator);

HBITMAP LoadImageFromResource(UINT resId, LPCTSTR resType);

int MsgBox(HWND hwnd, UINT boxFlags, LPCTSTR title, LPCTSTR fmtString, va_list argList);
int MsgBox(HWND hwnd, UINT boxFlags, LPCTSTR title, LPCTSTR fmtString, ...);
void ErrorBox(HWND hwnd, BOOL isCritical, LPCTSTR tfmtString, ...);
void ErrorBox(HWND hwnd, BOOL isCritical, UINT fmtStringId, ...);
int YesNoBox(HWND hwnd, LPCTSTR fmtString, ...);
int YesNoBox(HWND hwnd, UINT fmtStringId, ...);
int YesNoCancelBox(HWND hwnd, LPCTSTR fmtString, ...);
int YesNoCancelBox(HWND hwnd, UINT fmtStringId, ...);
void InfoBox(HWND hwnd, LPCTSTR fmtString, ...);
void InfoBox(HWND hwnd, UINT fmtStringId, ...);

tstring FileName(const tstring& filePath);
tstring FileExt(const tstring& filePath);
tstring DirName(const tstring& filePath);
BOOL IsWindowsXp();
BOOL IsCommCtrlVersion6();
BOOL FileExists(const tstring& fileName);
tstring FormatLastError();
tstring FormatBytes(int bytes);
tstring FormatLastInternetError();
void RegisterDialogClass(LPCTSTR newClassName);

void ListView_SetItemImage(HWND listView, int item, int subitem, int image);
int ListView_GetItemImage(HWND listView, int item, int subitem);
LPARAM ListView_GetItemParam(HWND listView, int index);
void ListView_SetHeaderSortImage(HWND listView, int columnIndex, BOOL isAscending);
void TabDlg_SetThemeBkgnd(HWND tabDlg);
LPARAM TabCtrl_GetItemParam(HWND hwndTab, int itemIndex);

tstring GetProgramFullPath();
tstring GetProgramDir();
HWND GetMainWindow(HWND hwnd);
HWND GetWindowsList(HWND hwnd);
HWND GetShellWindow();
HWND GetTrayWindow();

void BoldFont_Create(HWND hwnd);
void BoldFont_Delete(HWND hwnd);
HFONT BoldFont_Get(HWND hwnd);

void Windows_Create(HWND hwnd);
WindowsVector *Windows_Get(HWND hwnd);
void Windows_Destroy(HWND hwnd);

tstring Version_ToString(const VERSION& version);
