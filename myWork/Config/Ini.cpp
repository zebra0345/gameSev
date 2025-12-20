// Config/Ini.cpp
#include "Ini.h"
#include <Windows.h>
#include <cwchar>
#include <cstdlib>

// 실행위치 탐색
std::wstring GetExeDir()
{
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);

    std::wstring full = path;
    size_t pos = full.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? L"." : full.substr(0, pos);
}

// 문자열 읽어오기
std::wstring ReadIniWString(const wchar_t* section, const wchar_t* key,
    const wchar_t* defaultValue, const wchar_t* iniPath)
{
    wchar_t buf[256]{};
    GetPrivateProfileStringW(section, key, defaultValue, buf, 256, iniPath);
    return std::wstring(buf);
}

// 정수읽기
int ReadIniInt(const wchar_t* section, const wchar_t* key,
    int defaultValue, const wchar_t* iniPath)
{
    return (int)GetPrivateProfileIntW(section, key, defaultValue, iniPath);
}

// 실수 읽기
float ReadIniFloat(const wchar_t* section, const wchar_t* key,
    float defaultValue, const wchar_t* iniPath)
{
    wchar_t buf[64]{};
    GetPrivateProfileStringW(section, key, L"", buf, 64, iniPath);

    if (wcslen(buf) == 0)
        return defaultValue;

    return (float)_wtof(buf);
}

// 사각형 영역 읽기
RectF ReadIniRectF(const wchar_t* section, const wchar_t* key,
    const RectF& defaultValue, const wchar_t* iniPath)
{
    wchar_t buf[128]{};
    GetPrivateProfileStringW(section, key, L"", buf, 128, iniPath);

    if (wcslen(buf) == 0)
        return defaultValue;

    RectF r = defaultValue;
    swscanf_s(buf, L"%f,%f,%f,%f", &r.x, &r.y, &r.w, &r.h);
    return r;
}
