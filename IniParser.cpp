#include "resource.h"
#include "CommonFuncs.h"
#include "StringConversions.h"

#include "IniParser.h"
#define INITRUNKLENGTH          256


IniParser::IniParser()
{
}
IniParser::~IniParser()
{
}

void IniParser::FromFile(const tstring& iniFile)
{
    _groupsVector.clear();
    
    //Read file content
    FILE *file = _tfopen(iniFile.c_str(), _T("r"));
    if (!file)
        return;
    
    while(!feof(file))
    {
        //Get line
        TCHAR tline[4096] = _T("");
        _fgetts(tline, sizeof(tline), file);
        
        //Parse
        ParseLine(tline);
    }
    
    fclose(file);
}
void IniParser::FromString(const tstring& buffer)
{
    StringVector lines = explode(buffer, _T("\n"));
    for (int i = 0; i<lines.size(); i++)
        ParseLine(lines.at(i));
}
void IniParser::ToFile(const tstring& iniFile)
{
    //Erase previous content
    FILE *file = _tfopen(iniFile.c_str(), _T("w"));
    if (!file)
        return;
    
    int groupsCount = _groupsVector.size();
    for (int g = 0; g<groupsCount; g++)
    {
        _ftprintf(file, _T("[%s]\n"), _groupsVector.at(g).name.c_str());
        
        const IniKeyValueVector *keyValueVec = &_groupsVector.at(g).keyValueVec;
        int keyValueCount = keyValueVec->size();
        for (int kv = 0; kv<keyValueCount; kv++)
        {
            if (keyValueVec->at(kv).value.size() <= INITRUNKLENGTH)
            {
                _ftprintf(file, _T("%s=%s\n"), keyValueVec->at(kv).key.c_str(), 
                    keyValueVec->at(kv).value.c_str());
            }
            else
            {
                const tstring& value = keyValueVec->at(kv).value;
                
                //Split the value into many INITRUNKLENGTH long strings
                tstring trunk = _T("");
                for (int i = 0, start = 0;
                    start < value.size();
                    i++, start += INITRUNKLENGTH)
                {
                    trunk = value.substr(start, INITRUNKLENGTH);
                    _ftprintf(file, _T("%s%d=%s\n"), keyValueVec->at(kv).key.c_str(), i, trunk.c_str());
                }
            }
        }
    }
    
    fclose(file);
}

void IniParser::SetInt(LPCTSTR group, LPCTSTR key, int value)
{
    int groupIndex = FindGroup(group);
    if (groupIndex == -1)
    {
        groupIndex = AddGroup(group);
        AddKeyValue(groupIndex, key, i2a(value).c_str());
        return;
    }
    
    int keyIndex = FindKey(groupIndex, key);
    
    //Edit it
    if (keyIndex != -1)
        _groupsVector[ groupIndex ].keyValueVec[ keyIndex ].value = i2a(value);
    //Add it
    else
        AddKeyValue(groupIndex, key, i2a(value).c_str());
}
void IniParser::SetLong(LPCTSTR group, LPCTSTR key, long value)
{
    int groupIndex = FindGroup(group);
    if (groupIndex == -1)
    {
        groupIndex = AddGroup(group);
        AddKeyValue(groupIndex, key, l2a(value).c_str());
        return;
    }
    
    int keyIndex = FindKey(groupIndex, key);
    
    //Edit it
    if (keyIndex != -1)
        _groupsVector[ groupIndex ].keyValueVec[ keyIndex ].value = l2a(value);
    //Add it
    else
        AddKeyValue(groupIndex, key, l2a(value).c_str());
}
void IniParser::SetString(LPCTSTR group, LPCTSTR key, const tstring& value)
{
    int groupIndex = FindGroup(group);
    if (groupIndex == -1)
    {
        groupIndex = AddGroup(group);
        AddKeyValue(groupIndex, key, value.c_str());
        return;
    }
    
    int keyIndex = FindKey(groupIndex, key);
    
    //Edit it
    if (keyIndex != -1)
        _groupsVector[ groupIndex ].keyValueVec[ keyIndex ].value = value;
    //Add it
    else
        AddKeyValue(groupIndex, key, value.c_str());
}

int IniParser::GetInt(LPCTSTR group, LPCTSTR key, int defaultValue)
{
    int groupIndex = FindGroup(group);
    if (groupIndex == -1) return defaultValue;
    int keyIndex = FindKey(groupIndex, key);
    if (keyIndex == -1) return defaultValue;
    
    return a2i( _groupsVector[ groupIndex ].keyValueVec[ keyIndex ].value );
}
long IniParser::GetLong(LPCTSTR group, LPCTSTR key, long defaultValue)
{
    int groupIndex = FindGroup(group);
    if (groupIndex == -1) return defaultValue;
    int keyIndex = FindKey(groupIndex, key);
    if (keyIndex == -1) return defaultValue;
    
    return a2l( _groupsVector[ groupIndex ].keyValueVec[ keyIndex ].value );
}
tstring IniParser::GetString(LPCTSTR group, LPCTSTR key, const tstring& defaultValue)
{
    int groupIndex = FindGroup(group);
    if (groupIndex == -1) return defaultValue;
    
    int keyIndex = FindKey(groupIndex, key);
    if (keyIndex != -1)
        return _groupsVector[ groupIndex ].keyValueVec[ keyIndex ].value;
    else
    {
        tstring completeString = _T("");
        for (int i = 0; 
            (keyIndex = FindKey(groupIndex, strprintf(_T("%s%d"), key, i).c_str())) != -1;
            i++)
        {
            completeString += _groupsVector[ groupIndex ].keyValueVec[ keyIndex ].value;
        }
        
        if (completeString.empty())
            return defaultValue;
        return completeString;
    }
}

int IniParser::CountMatchingGroups(LPCTSTR group)
{
    int matchingGroupsCount = 0;
    
    int groupCount = _groupsVector.size();
    for (int i = 0; i<groupCount; i++)
    {
        if (_tcsstr(_groupsVector.at(i).name.c_str(), group))
            matchingGroupsCount++;
    }
    
    return matchingGroupsCount;
}
void IniParser::EraseMatchingGroups(LPCTSTR group)
{
    int groupCount = _groupsVector.size();
    for (int i = groupCount-1; i >= 0; i--)
    {
        if (_tcsstr(_groupsVector.at(i).name.c_str(), group))
            _groupsVector.erase( _groupsVector.begin() + i );
    } 
}
void IniParser::EraseGroup(LPCTSTR group)
{
    int groupIndex = FindGroup(group);
    if (groupIndex == -1) return;
    
    _groupsVector.erase( _groupsVector.begin() +groupIndex );
}
void IniParser::MoveGroup(LPCTSTR name, int index)
{
    int groupIndex = FindGroup(name);
    if (groupIndex == -1) return;
    if (index >= _groupsVector.size()) return;
    
    //Make a copy of this group
    const INIGROUP group = _groupsVector[ groupIndex ];
    
    //Erase the group
    _groupsVector.erase( _groupsVector.begin() + groupIndex );
    
    //Insert the group at the specified position
    _groupsVector.insert( _groupsVector.begin() + index, group );
}
BOOL IniParser::IsGroup(LPCTSTR name)
{
    return FindGroup(name) != -1;
}
std::vector<tstring> IniParser::GetGroupKeys(LPCTSTR group)
{
    std::vector<tstring> list;
    int groupIndex = FindGroup(group);
    if (groupIndex == -1) return list;
    
    int length = _groupsVector[groupIndex].keyValueVec.size();
    for (int i = 0; i < length; i++)
        list.push_back( _groupsVector[groupIndex].keyValueVec[i].key );
    
    return list;
}
void IniParser::EraseKey(LPCTSTR group, LPCTSTR key)
{
    int groupIndex = FindGroup(group);
    if (groupIndex == -1) return;
    
    int keyIndex = FindKey(groupIndex, key);
    if (keyIndex == -1) return;
    
    _groupsVector[groupIndex].keyValueVec.erase( 
        _groupsVector[groupIndex].keyValueVec.begin() +keyIndex );
}

void IniParser::ParseLine(tstring line)
{
    trim(line);
    if (line.empty())
        return;
    
    if (line.at(0) == _T('['))
    {
        int pos = line.find_last_of(_T(']'));
        if (pos == tstring::npos)
            return;
        
        //New group
        AddGroup( line.substr(1, pos-1).c_str() );
    }
    else
    {
        //No group, no key/value.
        if (_groupsVector.empty())
            return;
        
        //No =, no key
        int pos = line.find(_T('='));
        if (pos == tstring::npos)
            return;
        
        AddKeyValue( _groupsVector.size()-1, line.substr(0, pos).c_str(), 
            line.substr(pos+1, line.size() -pos -2).c_str() );
    }
}
int IniParser::AddGroup(LPCTSTR name)
{
    //Add a new group
    INIGROUP group;
    group.name = name;
    
    trim( group.name );
    if (!group.name.empty())
    {
        _groupsVector.push_back(group);
        return _groupsVector.size()-1;
    }
    
    return -1;
}
void IniParser::AddKeyValue(int groupIndex, LPCTSTR key, LPCTSTR value)
{
    INIKEYVALUE keyValue;
    keyValue.key = key;
    keyValue.value = value;
    
    trim( keyValue.key ); 
    trim( keyValue.value );
    if (!keyValue.key.empty())
    {
        if (groupIndex >= 0 && groupIndex < _groupsVector.size())
            _groupsVector[groupIndex].keyValueVec.push_back(keyValue);
    }
}
int IniParser::FindGroup(LPCTSTR group)
{
    int groupCount = _groupsVector.size();
    for (int i = 0; i<groupCount; i++)
    {
        if (_tcsicmp(group, _groupsVector.at(i).name.c_str()) == 0)
            return i;
    }
    
    return -1;
}
int IniParser::FindKey(int groupIndex, LPCTSTR key)
{
    const INIGROUP *group = &_groupsVector.at(groupIndex);
    int keyValueCount = group->keyValueVec.size();
    for (int i = 0; i<keyValueCount; i++)
    {
        if (_tcsicmp(key, group->keyValueVec.at(i).key.c_str()) == 0)
            return i;
    }
    
    return -1;
}

std::vector<tstring> IniParser::explode(tstring stringToExplode, const tstring& separator) 
{
    std::vector<tstring> explodedStrings;
    
    int sepPos = 0;
    while( (sepPos = stringToExplode.find(separator, 0)) != tstring::npos )
    {
        explodedStrings.push_back(stringToExplode.substr(0, sepPos));
        stringToExplode.erase(0, sepPos + separator.size());
    }
    
    if(stringToExplode != _T("") )
        explodedStrings.push_back(stringToExplode);
    
    return explodedStrings;
}
