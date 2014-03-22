#define CSS_FADEIN		    0x0001
#define CSS_FADEOUT		    0x0002
#define CSS_FADE		    CSS_FADEIN | CSS_FADEOUT
#define CSS_SHADOW		    0x0004
#define CSS_CENTERSCREEN	0x0008
#define CSS_CENTERAPP		0x0010

#define CSS_TEXT_NORMAL		0x0000
#define CSS_TEXT_BOLD		0x0001
#define CSS_TEXT_ITALIC		0x0002
#define CSS_TEXT_UNDERLINE	0x0004

class SplashScreen
{
    public:
        SplashScreen();
        ~SplashScreen();

        void Create(HWND hwndParent, LPCTSTR text = NULL, DWORD timeout = 2000, DWORD style = CSS_FADE|CSS_CENTERSCREEN|CSS_SHADOW);
        void Show();
        void Hide();
        void Invalidate();

        BOOL SetBitmap(HBITMAP bmp, short red = -1, short green = -1, short blue = -1);
        BOOL SetBitmap(UINT bitmapID, short red = -1, short green = -1, short blue = -1);
        BOOL SetBitmap(LPCTSTR fileName, short red = -1, short green = -1, short blue = -1);

        void SetText(LPCTSTR text);
        void SetTextFont(LPCTSTR fontName, int height, int style);
        void SetDefaultFont();
        void SetTextDefaultFont();
        void SetTextColor(COLORREF textColor);
        void SetTextRect(LPRECT rcText);
        void SetTextRect(int x, int y, int width, int height);
        void SetTextFormat(UINT textFormat);

    private:
        typedef std::basic_string<TCHAR> tstring;
        typedef BOOL (WINAPI* PANIMATEWINDOW)(HWND,DWORD,DWORD);

        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        HRGN CreateRgnFromBitmap(HBITMAP bmp, COLORREF color);
        void DrawWindow(HDC hdc);

        BOOL OnEraseBkgnd(HWND hwnd, HDC pDC);
        void OnPaint(HWND hwnd);
        LRESULT OnPrintClient(HWND hwnd, WPARAM wParam);


        HWND _splashScreen;

        PANIMATEWINDOW _pAnimateWindow;
        HWND _hwndParent;
        HBITMAP _bitmap;
        HFONT _customFont;
        HRGN _region;
        DWORD _style;
        DWORD _timeout;
        tstring _text;
        RECT _rcText;
        UINT _textFormat;
        COLORREF _textColor;
        int _bitmapWidth;
        int _bitmapHeight;
        int _xPos;
        int _yPos;
};
