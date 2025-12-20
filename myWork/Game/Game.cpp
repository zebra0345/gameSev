// Game/Game.cpp
#include "Game/Game.h"
#include "Core/Input.h"   // IsPressedOnce
#include <algorithm>      // std::min
#include <Windows.h> // OutputDebugStringW

// 디버그 메세지를 출력 창에 띄우는 도우미 함수
static void DebugLogW(const wchar_t* msg)
{
    OutputDebugStringW(msg);
}

// 개발 중 핫키를 눌러 랜더링 모드 변경하기
static void HandleDebugHotkeys(GameHooks& h)
{
    // hooks가 없으면 아무것도 하지 않는다.
    if (!h.isPressedOnce) return;

    if (h.debugFreezeAnim && h.isPressedOnce(VK_F3)) // 애니메이션 일시정지
        *h.debugFreezeAnim = !*h.debugFreezeAnim; 

    if (h.debugStepOnce && h.isPressedOnce(VK_F4)) // 1프레임씩 진행
        *h.debugStepOnce = true; // 1프레임 step 트리거

    if (h.debugOverlay && h.isPressedOnce(VK_F5)) // 정보 표시
        *h.debugOverlay = !*h.debugOverlay;

    if (h.debugBoxes && h.isPressedOnce(VK_F6)) // 상대 히트박스 표시
        *h.debugBoxes = !*h.debugBoxes;

    // F1누르면 도형 모드 <-> 스프라이트 전환
    // 주의 : 지금은 사용하고있지않음, 강제 Primitive
    if (h.renderMode && h.isPressedOnce(VK_F1))
    {
        *h.renderMode = (*h.renderMode == RenderMode::Primitive)
            ? RenderMode::SpriteRequested
            : RenderMode::Primitive;
    }
}

Game::Game()
{
    // 특별한 초기화는 Initialize()에서 수행
}

void Game::Bind(const GameHooks& hooks)
{
    hooks_ = hooks;
}

void Game::Initialize()
{
    QueryPerformanceFrequency(&freq_);
    QueryPerformanceCounter(&prev_);
    timerInitialized_ = true;
}

// 창을 드래그하거나 렉이 걸려 튀었을때 물리 계산 망가지는거 방지
static float ClampDt(float dt)
{
    // Alt-tab 등으로 dt가 튀면 물리/판정이 폭발하니까 상한
    const float kMax = 1.0f / 15.0f; // 최대 66ms
    if (dt < 0.f) dt = 0.f;
    if (dt > kMax) dt = kMax;
    return dt;
}

void Game::Tick()
{
    if (!timerInitialized_)
        Initialize();

    // ---- dt 계산 ----
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);

    double delta = double(now.QuadPart - prev_.QuadPart) / double(freq_.QuadPart);
    prev_ = now;

    float dtReal = ClampDt((float)delta);

    // ---- 방어적 바인딩 체크 (누락되면 로그 후 return) ----
    // null 참조방지
    if (!hooks_.renderMode || !hooks_.hitStopTimer || !hooks_.shakeTimer || !hooks_.timeAcc)
    {
        DebugLogW(L"[Game] Missing required state hooks (renderMode/timers)\n");
        return;
    }

    if (!hooks_.isPressedOnce || !hooks_.updatePlayer || !hooks_.updateEnemy || !hooks_.renderFrame)
    {
        DebugLogW(L"[Game] Missing required function hooks (input/update/render)\n");
        return;
    }

    // ---- 상태 업데이트 ----
    *hooks_.timeAcc += dtReal;

    // ---- Debug hotkeys (한 곳에서만) ----
    HandleDebugHotkeys(hooks_);

    // ---- Restart (플레이어가 죽었을 때만) ----
    if (hooks_.isPlayerDead && hooks_.isPlayerDead() && hooks_.isPressedOnce('R'))
    {
        if (hooks_.resetPlayer) hooks_.resetPlayer();
        if (hooks_.resetEnemy)  hooks_.resetEnemy();

        // 타이머/연출도 초기화
        *hooks_.hitStopTimer = 0.f;
        *hooks_.shakeTimer = 0.f;
    }

    // ---- HitStop 처리 ----
    if (*hooks_.hitStopTimer > 0.0f)
    {
        *hooks_.hitStopTimer -= dtReal;
        if (*hooks_.hitStopTimer < 0.0f)
            *hooks_.hitStopTimer = 0.0f;
    }

    // hitStop 중에는 연출(dtReal)
    float dtGame = (*hooks_.hitStopTimer > 0.0f) ? 0.0f : dtReal;

    // ---- 연출 업데이트 ----
    if (hooks_.updateCameraShake)    hooks_.updateCameraShake(dtReal);
    if (hooks_.updateFloatingTexts)  hooks_.updateFloatingTexts(dtReal);

    // ---- 게임 로직 업데이트 ----
    hooks_.updatePlayer(dtGame);
    hooks_.updateEnemy(dtGame);

    // ---- StepOnce는 한 번만 소비 ----
    if (hooks_.debugStepOnce)
        *hooks_.debugStepOnce = false;

    // ---- SpriteRequested는 현재 비활성 -> Primitive로 폴백 ----
    if (*hooks_.renderMode == RenderMode::SpriteRequested)
    {
        *hooks_.renderMode = RenderMode::Primitive;
        OutputDebugStringW(L"[Game::Tick] SpriteRequested -> fallback to Primitive\n");
    }

    // ---- 렌더 ----
    hooks_.renderFrame();
}

