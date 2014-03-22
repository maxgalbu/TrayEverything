#include "resource.h"
#include "HyperLinks.h"
#include "LangFuncs.h"
#include "MainFuncs.h"

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT titleFont = NULL;
    static HFONT boldFont = NULL;
    static HFONT middleTitleFont = NULL;
    static HICON mainIcon = NULL;
    
    switch (msg)
    {
        case WM_LANGUAGECHANGED:
        {
            Lang_DlgItemText(hwnd, IDH_AD_VERSION, IDS_AD_VERSION);
            
            Lang_DlgItemText(hwnd, IDC_AD_CHECKUPDATES, IDS_AD_CHECKUPDATES);
            Lang_DlgItemText(hwnd, IDH_AD_CREDITS, IDS_AD_CREDITS);
            Lang_DlgItemText(hwnd, IDC_AD_CREDITS, IDS_AD_CREDITS_EXT);
            Lang_DlgItemText(hwnd, IDC_AD_WEB, IDS_AD_WEBSITE);
        }
        break;
        
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_AD_WEB:
                    ShellExecute(NULL, NULL, _T("http://www.winapizone.net/software/trayeverything/"), NULL, NULL, SW_SHOW);
                    break;
               
                case IDC_AD_CHECKUPDATES:
                    SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDC_TB_UPDATE, 0), 0);
                    break;
            }
        }
        break;
        
        case WM_LBUTTONDOWN:
            MainDlg_MoveOnClick(hwnd);
            break;
        
        case WM_INITDIALOG:
        {
            SendMessage(hwnd, WM_LANGUAGECHANGED, 0,0);
            
            HFONT normalFont = GetStockFont(DEFAULT_GUI_FONT);
            
            {
                LOGFONT lf; GetObject(normalFont, sizeof(lf), &lf);
                lf.lfHeight = 34;
                lf.lfWeight = FW_BLACK;
                _tcscpy(lf.lfFaceName, _T("Verdana"));
                
                titleFont = CreateFontIndirect(&lf);
                SetDlgItemFont(hwnd, IDC_AD_HEADER, titleFont);
            }
            
            {
                LOGFONT lf; GetObject(normalFont, sizeof(lf), &lf);
                lf.lfWeight = FW_BOLD;
                
                boldFont = CreateFontIndirect(&lf);
                SetDlgItemFont(hwnd, IDC_AD_VERSION, boldFont);
            }
            
	        {
	            LOGFONT lf; GetObject(normalFont, sizeof(lf), &lf);
                lf.lfHeight = 17;
                lf.lfWeight = FW_HEAVY;
                
                middleTitleFont = CreateFontIndirect(&lf);
                SetDlgItemFont(hwnd, IDH_AD_CREDITS, middleTitleFont);
	        }
	        
	        mainIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAINICON));
	        SendDlgItemMessage(hwnd, IDC_AD_ICON, STM_SETICON, (WPARAM)mainIcon, 0);
	        
	        ConvertStaticToHyperlink(GetDlgItem(hwnd, IDC_AD_WEB));
        }
        break;
        
        case WM_DESTROY:
        {
            DeleteObject(titleFont);
            DeleteObject(boldFont);
            DeleteObject(middleTitleFont);
            DestroyIcon(mainIcon);
        }
        break;
    }
    return FALSE;
}
