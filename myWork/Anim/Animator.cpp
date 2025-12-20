// Anim/Animator.cpp
#define NOMINMAX
#include "Anim/Animator.h"
#include <algorithm>

// 클립을 바꾸면 시간/프레임/종료 상태를 초기화
void Animator::SetClip(const AnimClip& newClip)
{
    clip = newClip;
    time = 0.0f;
    currentFrame = 0;
    finished = false;
}

// 매 프레임 실행되어 애니메이션의 시간을 흐르게 함
void Animator::Update(float dt)
{
    if (finished) return; // 이미 끝났으면 종료
    if (clip.frameCount <= 1) return; // 프레임이 1개면 안움직임
    if (clip.fps <= 0.0f) return; // 속도 0이면 정지

    time += dt;

    const float frameTime = 1.0f / clip.fps; // 한프레임 시간
    int frame = (int)(time / frameTime); // 지난 시간으로 프레임 체크

    if (clip.loop)
    {   
        currentFrame = frame % clip.frameCount;  // 반복
    }
    else
    {
        // 반복안함 : 마지막 프레임에 도달시 멈춤
        if (frame >= clip.frameCount)
        {
            currentFrame = clip.frameCount - 1;  // 마지막 프레임에 고정
            finished = true;
        }
        else
        {
            currentFrame = frame;
        }
    }
}

// 스프라이트 시트 이미지에서 현재 프레임이 차지하는 픽셀 영역 계산
RECT Animator::GetSourceRectUnchecked(bool* outOutOfBounds) const
{
    RECT r{};

    // 현재 프레임이 실제 시트의 어느 칸인지 계산
    int col = clip.startCol + currentFrame;
    int row = clip.row;

    // 프레임 좌표 -> 픽셀좌표로 계산
    r.left = clip.originX + col * clip.frameW;
    r.top = clip.originY + row * clip.frameH;
    r.right = r.left + clip.frameW;
    r.bottom = r.top + clip.frameH;

    // extend 반영 (바깥 확장)
    r.left -= clip.extendL;
    r.right += clip.extendR;
    r.top -= clip.extendT;
    r.bottom += clip.extendB;

    // inset 반영 (안쪽 축소)
    if (clip.inset > 0)
    {
        r.left += clip.inset;
        r.top += clip.inset;
        r.right -= clip.inset;
        r.bottom -= clip.inset;
    }

    // 원본 이미지 벗어나는지 검사(안전하게)
    bool oob = false;
    if (r.left < 0 || r.top < 0 || r.right > clip.sheetW || r.bottom > clip.sheetH)
        oob = true;

    if (outOutOfBounds) *outOutOfBounds = oob;
    return r;
}

// 안전한 버전의 좌표 계산
RECT Animator::GetSourceRect() const
{
    bool dummy = false;
    RECT r = GetSourceRectUnchecked(&dummy);

    // 시트 범위를 무조건 벗어나지 않게 clamp
    if (r.left < 0) r.left = 0;
    if (r.top < 0) r.top = 0;
    if (r.right > clip.sheetW) r.right = clip.sheetW;
    if (r.bottom > clip.sheetH) r.bottom = clip.sheetH;

    return r;
}

// 프레임 강제 이동시키는 에디터, 혹은 특수 상황용에 사용
void Animator::StepFrame(int delta)
{
    if (clip.frameCount <= 1) return;

    int f = currentFrame + delta;

    if (clip.loop)
    {
        // 음수도 안전하게 순환하도록 보정
        f = (f % clip.frameCount + clip.frameCount) % clip.frameCount;
        currentFrame = f;
        finished = false;

        // time도 프레임에 맞춰서 되감기
        time = (float)currentFrame * (1.0f / std::max(clip.fps, 0.001f));
    }
    else
    {
        // 루프가 아니면 범위 고정
        if (f < 0) f = 0;
        if (f >= clip.frameCount) f = clip.frameCount - 1;

        currentFrame = f;
        finished = (currentFrame == clip.frameCount - 1);

        time = (float)currentFrame * (1.0f / std::max(clip.fps, 0.001f));
    }
}