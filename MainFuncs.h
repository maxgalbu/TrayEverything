void MainDlg_MoveOnClick(HWND hwnd);

UpdatesVector *Updates_Get(HWND hwnd);
void Updates_Remove(HWND hwnd);
void Updates_Set(HWND hwnd, UpdatesVector *newUpdatesVec);

void TrayIcon_CreateMenu(HWND hwnd, int index);
void TrayIcon_Set(HWND hwnd, int index);
void TrayIcon_Remove(HWND hwnd, int index);
BOOL TrayIcon_RemoveAll(HWND hwnd, BOOL restore = TRUE);

HICON Window_GetIcon(HWND window, int index);
void Window_Restore(HWND hwnd, int index);
void Window_Minimize(HWND hwnd, int index);

BOOL IsDayGone(HWND hwnd);
BOOL IsWeekGone(HWND hwnd);
void SaveLastUpdateDayDate(HWND hwnd);
void SaveLastUpdateWeekDate(HWND hwnd);

BOOL CALLBACK SetPasswordDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GetPasswordDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int FindWindow_FromUniqueId(HWND hwnd, unsigned long uniqueId);
int FindWindow_FromHandle(HWND hwnd, HWND window);
