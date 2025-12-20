// Config/Tuning.h
#pragma once
#include "Core/Types.h"

/*
  게임의 맛을 결정하는 미세한 숫자들 모음
  전역변수 gTune을 통해 어디서든 이 수치 참조
*/
struct GameTuning
{
    // =========================
    // Enemy
    // =========================
    int   enemyMaxHP = 5;
    float enemyChaseSpeed = 140.f;
    float enemyAggroRange = 320.f;
    float enemyStopRange = 70.f;
    float enemyBodyW = 90.f;
    float enemyBodyH = 130.f;

    // =========================
    // Combat
    // =========================
    float hitStunSec = 0.20f;
    float iFrameSec = 0.35f;
    float knockbackVx = 260.f;
    float knockbackDamp = 900.f;

    float hitStopSec = 0.06f;
    float shakeSec = 0.10f;
    float shakeAmp = 6.0f;

    float comboResetSec = 0.70f;

    // =========================
    // UI
    // =========================
    float hpBarW = 90.f;
    float hpBarH = 10.f;
    float hpBarOffsetY = 160.f;

    float floatTextLifeSec = 0.8f;
    float floatTextRiseSpeed = 40.f;

    // =========================
    // Enemy Attack 
    // =========================
    float enemyAttackCooldownSec = 1.0f;
    float enemyAttackWindupSec = 0.18f;
    float enemyAttackActiveSec = 0.10f;
    float enemyAttackRecoverSec = 0.25f;
    float enemyAttackRange = 85.f;
    RectF enemyAttackBoxLocal{ 150.f, 120.f, 90.f, 60.f };


    // Player combat
    float playerHitStunSec = 0.20f;
    float playerIFrameSec = 0.35f;
    float playerKnockbackVx = 260.f;
    float playerKnockbackDamp = 900.f;

    // Enemy combat
    float enemyHitStunSec = 0.20f;
    float enemyIFrameSec = 0.35f;
    float enemyKnockbackVx = 260.f;
    float enemyKnockbackDamp = 900.f;

    // Player movement/attack
    bool lockMoveDuringAttack = true;

    float inputBufferSec = 0.12f;
};

// 전역 튜닝
extern GameTuning gTune;

// tuning.ini 로드
void LoadTuningFromIni();
