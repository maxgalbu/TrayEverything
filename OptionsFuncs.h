void Options_Load(HWND hwnd);
void Options_Save(HWND hwnd);
void Options_Destroy(HWND hwnd);
void Options_Restore(HWND hwnd);

int Options_GetInt(HWND hwnd, int option);
tstring Options_GetString(HWND hwnd, int option);
IntVector *Options_GetProgramOptionsVector(HWND hwnd, tstring program);
StringVector Options_GetProgramPathsVector(HWND hwnd);

void Options_SetInt(HWND hwnd, int option, int value);
void Options_SetString(HWND hwnd, int option, tstring value);
void Options_SetProgramOptionsVector(HWND hwnd, tstring program, IntVector *options);
