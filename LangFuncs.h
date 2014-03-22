BOOL GetNextLanguageDll(HANDLE& fileHandle, LPTSTR filePath);

BOOL Lang_Load(HWND hwnd, BOOL recover = FALSE);
void Lang_Unload(HWND hwnd);

tstring Lang_LoadString(HWND hwnd, UINT stringId);

inline BOOL Lang_WindowText(HWND hwnd, UINT stringId)
{
    return SetWindowText(hwnd, Lang_LoadString(hwnd, stringId).c_str());
}
inline BOOL Lang_DlgItemText(HWND parent, UINT id, UINT stringId)
{
    return SetDlgItemText(parent, id, Lang_LoadString(parent, stringId).c_str());
}
inline BOOL Lang_AppendMenu(HWND hwnd, HMENU menu, UINT flags, UINT id, UINT stringId)
{
    return AppendMenu(menu, flags, id, Lang_LoadString(hwnd, stringId).c_str());
}
