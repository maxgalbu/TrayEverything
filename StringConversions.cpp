#include "resource.h"
#include <math.h> //for modf

std::wstring a2w(const std::string& cString)
{
    int strLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cString.c_str(), -1, NULL, 0);
    WCHAR *wString = new WCHAR[strLen];
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cString.c_str(), -1, wString, strLen);
    
    std::wstring wideStr = wString;
    delete [] wString;
    return wideStr;
}
std::string w2a(const std::wstring& wString)
{
    int wideStrLen = WideCharToMultiByte(CP_ACP, 0, wString.c_str(), -1, NULL, 0, NULL, NULL);
    CHAR *cString = new CHAR[wideStrLen];
    WideCharToMultiByte(CP_ACP, 0, wString.c_str(), -1, cString, wideStrLen, NULL, NULL);
    
    std::string mbStr = cString;
    delete [] cString;
    return mbStr;
}

tstring i2a(int number)
{
    TCHAR text[50];
    _itot(number, text, 10);
    return text;
}
tstring l2a(long number)
{
    TCHAR text[50];
    _ltot(number, text, 10);
    return text;
}
tstring d2a(double number)
{
    //Get the integer part
    double intPart;
    modf(number, &intPart);
    
    //If this number has no decimal part, 
    //return the integer value
    if (intPart == number)
        return i2a((int)number);
    
    TCHAR text[50];
    _stprintf(text, _T("%.2f"), number);
    return text;
}

int a2i(const tstring& string)
{
    return _ttoi(string.c_str());
}
long a2l(const tstring& string)
{
    return _ttol(string.c_str());
}
double a2d(const tstring& string)
{
    return _tcstod(string.c_str(), NULL);
}
