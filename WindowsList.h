BOOL CALLBACK WindowsListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void WindowsList_OnColumnClick(NMLISTVIEW *nmlv);
void WindowsList_OnHotTrack(NMLISTVIEW *nmlv);
void WindowsList_OnLeftClick(NMITEMACTIVATE *nmia);
void WindowsList_OnDoubleClick(NMITEMACTIVATE *nmia);
void WindowsList_OnRightClick(NMITEMACTIVATE *nmia);

void WindowsList_Init(HWND listView);
void WindowsList_Update(HWND listView);
void WindowsList_LanguageChanged(HWND listView);
