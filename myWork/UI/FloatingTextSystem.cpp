// UI/FloatingTextSystem.cpp
#include "FloatingTextSystem.h"
#include "Config/Tuning.h"
#include <algorithm>

// 현재 화면에 떠 있는 모든 객체들 보관 리스트
static std::vector<FloatingText> sTexts;

// 텍스트 생성하여 리스트에 추가
void SpawnFloatingText(float x, float y, const std::wstring& text)
{
    if (text.empty()) return;

    FloatingText t;
    t.text = text;
    t.x = x;
    t.y = y;
    t.life = gTune.floatTextLifeSec;

    sTexts.push_back(t);
}

// 텍스트의 상태를 업데이트하고 수명이 다한 데이터 정리
void UpdateFloatingTexts(float dtReal)
{
    // 모든 텍스트의 위치와 수명 갱신
    for (auto& t : sTexts)
    {
        t.life -= dtReal; // 남은 수명을 시간만큼 차단
        t.y -= gTune.floatTextRiseSpeed * dtReal; // 위로 떠올리는 효과 연출
    }

    // 수명이 다한 텍스트 제거
    // 성능 저하가 없도록 최적화 방식
    sTexts.erase(
        std::remove_if(sTexts.begin(), sTexts.end(),
            [](const FloatingText& t) { return t.life <= 0.0f; }),
        sTexts.end()
    );
}

// 모든 데이터 추가
void ClearFloatingTexts()
{
    sTexts.clear();
}

// 외부에 데이터 전달
const std::vector<FloatingText>& GetFloatingTexts()
{
    return sTexts;
}
