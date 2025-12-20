// Render/Renderer.h
#pragma once
#include <vector>
#include "Core/AppTypes.h"

// 전방 선언 : 구체적인 구조는 몰라도 포인터로 가리킬 수 있게 이름만 알려줌
struct Player;
struct Enemy;

// Renderer가 외부(=main) 기능을 이름으로 링크하지 않게
// 함수 포인터로 의존성 분리
struct RenderCallbacks
{
    void (*renderWorld)() = nullptr; // 월드(플레이어/적/박스 등)
    void (*renderPlayerHP)() = nullptr; // 플레이어 체력 게이지
    void (*renderEnemyHPBar)() = nullptr; // 적 체력 게이지
    void (*drawDebugOverlay)() = nullptr; // 성능 지표 등 디버그 정보
    void (*present)() = nullptr; // 그린 내용을 실제 모니터로 전송
    
    // 떠다니는 텍스트를 그리는 기능
    void (*drawFloatingTexts)(const std::vector<FloatingText>&) = nullptr;
};

// RenderFrame에서 그릴 것들을 모아 입력으로 받는다.
struct RenderInput
{
    const Player* player = nullptr; // 플레이어 상태 정보
    const Enemy* enemy = nullptr; // 적 상태 정보

    const std::vector<FloatingText>* floatingTexts = nullptr; // 현재 떠있는 텍스트

    RenderMode mode = RenderMode::Primitive; // 기본 랜더링 모드

    bool debugOverlay = false; // 디버그 창을 보여줄지 말지
    bool debugBoxes = false; // 히트박스 보여줄지 말지

    RenderCallbacks cb{}; // 그리기 함수 도구
};

// 외부(main)에서 호출하여 렌더링 공정을 시작하는 진입점
void RenderFrame_Primitive(const RenderInput& in);
