// Config/Input.h
#pragma once

// 키가 "현재" 눌려있는지
bool IsDown(int vk);

// 키가 "이번 프레임에 처음 눌렸는지" (토글/단발 입력에 사용)
bool IsPressedOnce(int vk);
