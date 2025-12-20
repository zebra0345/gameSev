// Core/AppTypes.h
#pragma once
#include <vector>
#include <string>

// 랜더 모드 선언
// 기본 사각형일지, 스프라이트로 보여질지
enum class RenderMode
{
    Primitive, // 사각형 모드
    SpriteRequested // 스프라이트 모드
};

// 다른 파일에서 사용하니 extern 선언
extern RenderMode gRenderMode;

// 데미지 글자의 정보
struct FloatingText
{
    std::wstring text; // 글자내용
    float x = 0.f; // 글자가 나타날 x좌표
    float y = 0.f; // 글자가 나타날 y좌표
    float life = 0.f; // 글자가 화면에 머무르는 시간
};
