// UI/FloatingTextSystem.h
#pragma once
#include <vector>
#include <string>
#include "Core/AppTypes.h"

// 어디서든 호출 가능 (데미지/히트 연출 등)
void SpawnFloatingText(float x, float y, const std::wstring& text);

// 매 프레임 업데이트 (life 감소 / rise)
void UpdateFloatingTexts(float dtReal);

// 리셋 시 정리용
void ClearFloatingTexts();

// Render에서 읽기 전용 접근
const std::vector<FloatingText>& GetFloatingTexts();
