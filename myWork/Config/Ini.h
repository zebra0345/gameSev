// Config/Ini.h
#pragma once
#include <string>
#include "Core/Types.h"

// 실행 파일 위치 기준으로 디렉터리 얻기
std::wstring GetExeDir();

// ini에서 문자열 읽기
std::wstring ReadIniWString(const wchar_t* section, const wchar_t* key,
    const wchar_t* defaultValue, const wchar_t* iniPath);

// ini에서 int 읽기
int ReadIniInt(const wchar_t* section, const wchar_t* key,
    int defaultValue, const wchar_t* iniPath);

// ini에서 float 읽기
float ReadIniFloat(const wchar_t* section, const wchar_t* key,
    float defaultValue, const wchar_t* iniPath);

// ini에서 RectF( x,y,w,h ) 읽기
RectF ReadIniRectF(const wchar_t* section, const wchar_t* key,
    const RectF& defaultValue, const wchar_t* iniPath);
