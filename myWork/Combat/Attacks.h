// Combat/Attacks.h
#pragma once
#include <string>
#include <unordered_map>

#include "Core/Types.h"     // RectF
#include "Anim/Animator.h"  // AnimClip

// 공격 하나의 정의(1단, 2단)
struct AttackDef
{
    std::wstring name;      // "Attack1" 같은 키
    AnimClip clip;          // 애니메이션 + hitframe + hitbox까지 포함
    int chainFrame = -1;    // 이 프레임부터 연계 입력 허용
    std::wstring next;      // 다음 공격 이름 (Attack2 등)
    std::wstring stageText; // 디버그/연출용 문자열
};

// 공격 데이터들을 전역적으로 관리하고 로드하는 매니저
namespace Attacks
{
    // attacks.ini 로드 (성공하면 true)
    bool LoadFromFile(const std::wstring& path);

    // 못 찾으면 nullptr
    const AttackDef* Find(const std::wstring& name);

    // 디버그용: 전체 비우기
    void Clear();

    // 디버그용: 개수
    int Count();

    // 로드 실패 대비함
    void InstallFallbackDefaults();
}