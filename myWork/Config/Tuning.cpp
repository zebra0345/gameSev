// Config/Tuning.cpp
#include "Tuning.h"
#include "Ini.h"
#include <Windows.h>

// 실제 데이터 저장 공간 생성
GameTuning gTune;

void LoadTuningFromIni()
{
    std::wstring iniPath = GetExeDir() + L"\\assets\\tuning.ini";
    if (GetFileAttributesW(iniPath.c_str()) == INVALID_FILE_ATTRIBUTES)
        return;

    // Enemy
    gTune.enemyMaxHP = ReadIniInt(L"Enemy", L"MaxHP", gTune.enemyMaxHP, iniPath.c_str());
    gTune.enemyChaseSpeed = ReadIniFloat(L"Enemy", L"ChaseSpeed", gTune.enemyChaseSpeed, iniPath.c_str());
    gTune.enemyAggroRange = ReadIniFloat(L"Enemy", L"AggroRange", gTune.enemyAggroRange, iniPath.c_str());
    gTune.enemyStopRange = ReadIniFloat(L"Enemy", L"StopRange", gTune.enemyStopRange, iniPath.c_str());
    gTune.enemyBodyW = ReadIniFloat(L"Enemy", L"BodyW", gTune.enemyBodyW, iniPath.c_str());
    gTune.enemyBodyH = ReadIniFloat(L"Enemy", L"BodyH", gTune.enemyBodyH, iniPath.c_str());

    // Combat
    gTune.hitStunSec = ReadIniFloat(L"Combat", L"HitStunSec", gTune.hitStunSec, iniPath.c_str());
    gTune.iFrameSec = ReadIniFloat(L"Combat", L"IFrameSec", gTune.iFrameSec, iniPath.c_str());
    gTune.knockbackVx = ReadIniFloat(L"Combat", L"KnockbackVx", gTune.knockbackVx, iniPath.c_str());
    gTune.knockbackDamp = ReadIniFloat(L"Combat", L"KnockbackDamp", gTune.knockbackDamp, iniPath.c_str());

    gTune.hitStopSec = ReadIniFloat(L"Combat", L"HitStopSec", gTune.hitStopSec, iniPath.c_str());
    gTune.shakeSec = ReadIniFloat(L"Combat", L"ShakeSec", gTune.shakeSec, iniPath.c_str());
    gTune.shakeAmp = ReadIniFloat(L"Combat", L"ShakeAmp", gTune.shakeAmp, iniPath.c_str());

    gTune.comboResetSec = ReadIniFloat(L"Combat", L"ComboResetSec", gTune.comboResetSec, iniPath.c_str());

    // UI
    gTune.hpBarW = ReadIniFloat(L"UI", L"HPBarW", gTune.hpBarW, iniPath.c_str());
    gTune.hpBarH = ReadIniFloat(L"UI", L"HPBarH", gTune.hpBarH, iniPath.c_str());
    gTune.hpBarOffsetY = ReadIniFloat(L"UI", L"HPBarOffsetY", gTune.hpBarOffsetY, iniPath.c_str());

    // EnemyAttack (없으면 기본값 유지)
    gTune.enemyAttackCooldownSec = ReadIniFloat(L"EnemyAttack", L"CooldownSec", gTune.enemyAttackCooldownSec, iniPath.c_str());
    gTune.enemyAttackWindupSec = ReadIniFloat(L"EnemyAttack", L"WindupSec", gTune.enemyAttackWindupSec, iniPath.c_str());
    gTune.enemyAttackActiveSec = ReadIniFloat(L"EnemyAttack", L"ActiveSec", gTune.enemyAttackActiveSec, iniPath.c_str());
    gTune.enemyAttackRecoverSec = ReadIniFloat(L"EnemyAttack", L"RecoverSec", gTune.enemyAttackRecoverSec, iniPath.c_str());
    gTune.enemyAttackRange = ReadIniFloat(L"EnemyAttack", L"Range", gTune.enemyAttackRange, iniPath.c_str());

    gTune.enemyAttackBoxLocal = ReadIniRectF(L"EnemyAttack", L"AttackBoxLocal",
        gTune.enemyAttackBoxLocal, iniPath.c_str());
}
