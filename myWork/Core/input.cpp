// Config/Input.cpp
#include "Input.h"
#include <Windows.h>

// 내부 static 상태는 여기서만 유지
static bool gPrevKeyState[256] = {};

// 키가 현재 내려가있는지
bool IsDown(int vk)
{
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

// 새로 눌렸는지
bool IsPressedOnce(int vk)
{
    SHORT state = GetAsyncKeyState(vk);
    bool isDown = (state & 0x8000) != 0;

    bool pressedOnce = isDown && !gPrevKeyState[vk];
    gPrevKeyState[vk] = isDown;

    return pressedOnce;
}
