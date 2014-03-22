typedef struct tagINIKEYVALUE
{
    tstring key;
    tstring value;
} INIKEYVALUE;
typedef std::vector<INIKEYVALUE> IniKeyValueVector;
typedef struct tagINIGROUP
{
    tstring name;
    IniKeyValueVector keyValueVec;
} INIGROUP;
typedef std::vector<INIGROUP> IniGroupVec;

class IniParser
{
    public:
        IniParser();
        ~IniParser();
        
        void FromFile(const tstring& iniFile);
        void FromString(const tstring& buffer);
        void ToFile(const tstring& iniFile);
        
        void SetInt(LPCTSTR group, LPCTSTR key, int value);
        void SetLong(LPCTSTR group, LPCTSTR key, long value);
        void SetString(LPCTSTR group, LPCTSTR key, const tstring& value = _T(""));
        
        int GetInt(LPCTSTR group, LPCTSTR key, int defaultValue);
        long GetLong(LPCTSTR group, LPCTSTR key, long defaultValue);
        tstring GetString(LPCTSTR group, LPCTSTR key, const tstring& defaultValue = _T(""));
        
        int CountMatchingGroups(LPCTSTR group);
        void EraseMatchingGroups(LPCTSTR group);
        void EraseGroup(LPCTSTR group);
        void MoveGroup(LPCTSTR name, int index);
        BOOL IsGroup(LPCTSTR name);
        std::vector<tstring> GetGroupKeys(LPCTSTR group);
        void EraseKey(LPCTSTR group, LPCTSTR key);
        
    private:
        IniGroupVec _groupsVector;
        
        void ParseLine(tstring line);
        
        int AddGroup(LPCTSTR name);
        void AddKeyValue(int groupIndex, LPCTSTR key, LPCTSTR value);
        int FindGroup(LPCTSTR group);
        int FindKey(int groupIndex, LPCTSTR key);
        
        std::vector<tstring> explode(tstring stringToExplode, const tstring& separator);
};
