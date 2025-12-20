// Game/GameHooks.h
#pragma once
#include <vector>
#include "Core/AppTypes.h"

struct Player;
struct Enemy;

struct GameHooks
{
    // ---- pointers to game state ----
    RenderMode* renderMode = nullptr;

    Player* player = nullptr;
    Enemy*  enemy  = nullptr;

    // UI data (ÀÐ±â/clear ¿ë)
    std::vector<FloatingText>* floatingTexts = nullptr;

    // ---- debug flags ----
    bool* debugFreezeAnim = nullptr;
    bool* debugStepOnce   = nullptr;
    bool* debugOverlay    = nullptr;
    bool* debugBoxes      = nullptr;

    // ---- timers ----
    float* hitStopTimer = nullptr;
    float* shakeTimer   = nullptr;
    float* timeAcc      = nullptr;

    // ---- input ----
    bool (*isPressedOnce)(int vk) = nullptr;
    bool (*isPlayerDead)() = nullptr;

    // ---- updates ----
    void (*updateCameraShake)(float dtReal)   = nullptr;
    void (*updateFloatingTexts)(float dtReal) = nullptr;
    void (*updatePlayer)(float dtGame)        = nullptr;
    void (*updateEnemy)(float dtGame)         = nullptr;

    // ---- render ----
    void (*renderFrame)() = nullptr;

    // ---- reset ----
    void (*resetPlayer)() = nullptr;
    void (*resetEnemy)()  = nullptr;
};
