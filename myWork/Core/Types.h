// Core/Types.h - 기본 물리/수학정의
#pragma once
#include <Windows.h>
#include <cmath>

// RectF 구조체
// 게임의 모든 영역 표헌
struct RectF
{
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;
};

// 두 물체(예: 내 칼과 적의 몸통)가 겹쳤는지 확인
inline bool Intersect(const RectF& a, const RectF& b)
{
    return (a.x < b.x + b.w) && (a.x + a.w > b.x) &&
        (a.y < b.y + b.h) && (a.y + a.h > b.y);
}

// 게임 내의 정밀한 소수점 좌표를 화면 출력용 정수로 치환
inline RECT ToRECT(const RectF& r)
{
    RECT out{};
    out.left = (LONG)floorf(r.x + 0.5f);
    out.top = (LONG)floorf(r.y + 0.5f);
    out.right = (LONG)floorf((r.x + r.w) + 0.5f);
    out.bottom = (LONG)floorf((r.y + r.h) + 0.5f);
    return out;
}
