// Core/AppTypes.cpp
#include "Core/AppTypes.h"

// 전역정의
// 화면 출력 모드의 기본값 = Primitive(사각형)
// 전역정의로 어디든 랜더링 상태 확인 후 변경가능
RenderMode gRenderMode = RenderMode::Primitive;
