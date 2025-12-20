// Game/Game.h
#pragma once
#include <Windows.h>
#include "GameHooks.h"

class Game
{
public:
    Game();

    // 외부 시스템이나 변수를 연결
    void Bind(const GameHooks& hooks);
    // 정밀 타이머 등 초기화
    void Initialize();
    // 매 프레임마다 호출
    void Tick();

private:
    GameHooks hooks_{}; // 열결된 외부 시스템의 참조

    LARGE_INTEGER freq_{}; // 성능측정(진동수)
    LARGE_INTEGER prev_{}; // 이전 프레임의 시간 기록
    bool timerInitialized_ = false;
};
