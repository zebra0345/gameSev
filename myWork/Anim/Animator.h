// Anim/Animator.h
#pragma once

// 이 파일은 스프라이트 애니메이션의 재생만 담당

#include <string>
#include <algorithm>     // std::max(수치계산)
#include <Windows.h>

#include "Core/Types.h"  // 타입 가져와서 사용

// 걷기, 공격 등 특정 동작 하나에 대한 원본 소스 정보 담는 구조체
struct AnimClip
{
    // 스프라이트 한 프레임 크기
    int frameW = 0;
    int frameH = 0;

    // 시트 상에서 행(row) / 시작 열(startCol)
    int row = 0;
    int startCol = 0;

    // 총 프레임 수, FPS, 루프 여부
    int frameCount = 1;
    float fps = 8.0f;
    bool loop = true;

    // bleeding 방지용 보정
    int inset = 0;   // 안쪽으로 줄이는 픽셀
    int extendL = 0; // 바깥으로 확장 픽셀
    int extendR = 0;
    int extendT = 0;
    int extendB = 0;

    // 피벗(프레임 로컬 기준)
    int pivotX = 128;
    int pivotY = 230;

    // 공격 유효 프레임과 히트박스(클립 로컬 좌표)
    int hitStartFrame = -1;
    int hitEndFrame = -1;
    RectF hitBoxLocal{};

    // 시트 전체 크기(범위 검사에 사용)
    int sheetW = 1536;
    int sheetH = 1024;

    // 시트 원점 보정(대부분 0,0 유지)
    int originX = 0;
    int originY = 0;
};

// "클립을 재생"하는 순수 애니메이터
struct Animator
{
    AnimClip clip{};
    float time = 0.0f;       // 누적 시간
    int currentFrame = 0;    // 현재 프레임 index
    bool finished = false;   // 루프 아닌 경우 종료 여부

    // 새로운 애니메이션으로 바꿀 때 사용
    void SetClip(const AnimClip& newClip);
    // 매 프레임마다 시간 업데이트, 프레임 계산
    void Update(float dt);

    // 시트에서 잘라올 SourceRect. outOutOfBounds로 범위 밖인지도 알려줌.
    RECT GetSourceRectUnchecked(bool* outOutOfBounds) const;

    // 무조건 clamp된 안전한 SourceRect
    RECT GetSourceRect() const;

    // 디버그용: 프레임을 수동으로 한 칸 이동
    void StepFrame(int delta);
};
